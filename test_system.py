"""Integration tests using only temporary folders, Python 3 and GCC."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent


class ExpenseTrackerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory()
        cls.program = Path(cls.build.name) / "expense_tracker"
        subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                        str(ROOT / "main.c"), "-o", str(cls.program)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.build.cleanup()

    def setUp(self):
        self.workspace = tempfile.TemporaryDirectory()
        self.folder = Path(self.workspace.name)

    def tearDown(self):
        self.workspace.cleanup()

    def run_program(self, text, success=True):
        result = subprocess.run([str(self.program)], input=text, text=True,
                                capture_output=True, cwd=self.folder, timeout=5)
        self.assertEqual(result.returncode, 0 if success else 1, result.stderr)
        return result.stdout + result.stderr

    def add(self):
        return self.run_program("1\n1\n2026-01-05\n1\n120\nLunch\n0\n")

    def test_empty_and_eof(self):
        self.assertIn("No expenses found", self.run_program("2\n"))
        self.assertFalse((self.folder / "expenses.txt").exists())

    def test_add_search_and_restart(self):
        self.assertIn("Expense added and saved", self.add())
        output = self.run_program("3\n1\n0\n")
        for value in ("2026-01-05", "Food", "120.00", "Lunch"):
            self.assertIn(value, output)

    def test_duplicate_and_missing(self):
        self.add()
        output = self.run_program("1\n1\n3\n999\n4\n999\n5\n999\n0\n")
        self.assertIn("This ID already exists", output)
        self.assertEqual(output.count("Expense not found"), 3)

    def test_update_persists(self):
        self.add()
        self.assertIn("Expense updated", self.run_program("4\n1\n2026-02-06\n2\n150\nTaxi fare\n0\n"))
        output = self.run_program("3\n1\n0\n")
        for value in ("2026-02-06", "Travel", "150.00", "Taxi fare"):
            self.assertIn(value, output)

    def test_delete_cancel_and_confirm(self):
        self.add()
        self.assertIn("Delete cancelled", self.run_program("5\n1\n0\n0\n"))
        self.assertIn("Lunch", self.run_program("2\n0\n"))
        self.assertIn("Expense deleted", self.run_program("5\n1\n1\n0\n"))
        self.assertIn("No expenses found", self.run_program("2\n0\n"))

    def test_summary_filters_month_and_year(self):
        (self.folder / "expenses.txt").write_text(
            "1|2026-01-05|0|120|Lunch\n2|2026-01-06|1|80|Bus\n"
            "3|2026-02-05|2|999|Shoes\n4|2025-01-05|0|999|Old meal\n")
        output = self.run_program("6\n2026\n1\n0\n")
        for value in ("60.0%", "40.0%", "Total: INR 200.00", "Average per expense: INR 100.00",
                      "Highest spending category: Food", "Records: 2"):
            self.assertIn(value, output)
        self.assertIn("No expenses for this month", self.run_program("6\n2026\n3\n0\n"))

    def test_dates_and_leap_years(self):
        for date, valid in (("2024-02-29", True), ("2000-02-29", True), ("1900-02-29", False),
                            ("2026-02-29", False), ("2026-04-31", False), ("2026-13-01", False),
                            ("2026-1-01", False), ("1899-12-31", False), ("9999-12-31", True)):
            with self.subTest(date=date):
                (self.folder / "expenses.txt").write_text(f"1|{date}|0|10|Test\n")
                output = self.run_program("0\n", success=valid)
                if not valid:
                    self.assertIn("Invalid or duplicate record", output)

    def test_invalid_input_and_partial_add(self):
        output = self.run_program("abc\n9\n1.5\n1\n0\n-1\n1\n2026-02-30\n2026-01-05\n0\n7\n1\n0\n-1\nnan\ninf\n12x\n100000001\n120\n\nA|B\nLunch\n0\n")
        self.assertIn("Expense added", output)
        before = (self.folder / "expenses.txt").read_text()
        self.run_program("1\n2\n2026-01-06\n2\n")
        self.assertEqual((self.folder / "expenses.txt").read_text(), before)

    def test_overlong_description(self):
        output = self.run_program("1\n1\n2026-01-05\n1\n120\n" + "X" * 100 + "\nLunch\n0\n")
        self.assertIn("Input is too long", output)
        self.assertIn("Expense added", output)

    def test_corrupt_file_preserved(self):
        for data in ("garbage\n", "1|2026-01-01|0|nan|Meal\n", "1|2026-01-01|8|10|Meal\n",
                     "999999999999999999|2026-01-01|0|10|Meal\n",
                     "1|2026-01-01|0|10|A\n1|2026-01-02|0|20|B\n"):
            (self.folder / "expenses.txt").write_text(data)
            self.assertIn("Invalid or duplicate record", self.run_program("", success=False))
            self.assertEqual((self.folder / "expenses.txt").read_text(), data)

    def test_failed_save_rolls_back(self):
        self.add()
        before = (self.folder / "expenses.txt").read_text()
        (self.folder / "expenses.tmp").mkdir()
        output = self.run_program("1\n2\n2026-01-06\n2\n80\nBus\n4\n1\n2026-02-01\n3\n500\nShoes\n5\n1\n1\n3\n1\n0\n")
        for phrase in ("Expense not added", "Update cancelled", "Delete cancelled", "Lunch", "120.00"):
            self.assertIn(phrase, output)
        self.assertEqual((self.folder / "expenses.txt").read_text(), before)

    def test_capacity_limit(self):
        data = "".join(f"{i}|2026-01-01|0|1|Test\n" for i in range(1, 1001))
        (self.folder / "expenses.txt").write_text(data)
        self.assertIn("Expense limit reached", self.run_program("1\n0\n"))

    def test_backup_guard(self):
        (self.folder / "expenses.bak").write_text("1|2026-01-01|0|120|Lunch\n")
        self.assertIn("Restore expenses.bak", self.run_program("", success=False))


if __name__ == "__main__":
    unittest.main(verbosity=2)

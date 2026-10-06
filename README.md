# Smart Expense Tracker using C

**Author: Gurleen Kaur**

In this project, I record daily expenses and summarise monthly spending through a menu-driven console program. I use C structures, arrays, functions, input validation and file handling.

## Features

- Add an expense with a unique ID, date, category, amount and description.
- View all records with a total.
- Search, update and delete expenses by ID.
- Confirm before deleting a record.
- Show monthly totals by category, category percentages, average expense and the highest spending category.
- Save each successful change to a text file and load records on restart.
- Validate duplicate IDs, positive amounts, real calendar dates and text lengths.
- Roll back changes when saving fails; stop safely when stored records are invalid.

Categories: Food, Travel, Shopping, Bills, Education and Other. Amounts are in INR.

## Compile and run

Install GCC and open a terminal in the project folder.

**Linux / macOS (with GCC installed):**

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o expense_tracker
./expense_tracker
```

**Windows (MinGW GCC):**

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o expense_tracker.exe
.\expense_tracker.exe
```

No third-party C libraries are required. Run from a writable folder: `expenses.txt` is stored in the current working directory. To start with fictional demo records, copy `sample-expenses.txt` to `expenses.txt` first. Do not overwrite a data file you want to keep.

## Menu

```text
1. Add expense
2. View all expenses
3. Search expense
4. Update expense
5. Delete expense
6. Monthly summary
0. Exit
```

## How it works

1. The `Expense` structure groups an ID, date, category, amount and description. An array stores up to 1,000 records; `expenseCount` tells us how many slots are used.
2. `main()` loads the file, displays the menu repeatedly and calls the function for each operation.
3. `findExpense()` checks records one at a time. It returns the matching array index or `-1`. Search is O(n).
4. `readLine()` uses `fgets()` for whole-line input. `strtol()` and `strtod()` validate numbers without leaving bad input in the input buffer.
5. `validDate()` checks `YYYY-MM-DD`, month lengths and leap years. A year divisible by 400 is a leap year; other century years are not; remaining years divisible by 4 are leap years.
6. `monthlySummary()` matches the first seven date characters (`YYYY-MM`) and adds amounts to a category-total array. Percentage = category total / month total x 100. Average = month total / number of expenses.
7. Updates keep the original ID. Deleting a record shifts later array elements left to close the gap.
8. `saveExpenses()` writes a temporary file before replacing the original. Each CRUD function restores its previous in-memory state if saving fails.

## File format

One record per line:

```text
ID|YYYY-MM-DD|CategoryIndex|Amount|Description
```

For example:

```text
1|2026-01-05|0|120|Lunch
2|2026-01-06|1|80|Bus fare
3|2026-01-07|4|500|C programming book
```

The category index is zero-based (0 = Food, 1 = Travel, ..., 5 = Other). The menu shows 1 through 6 for easier selection. The description cannot contain `|`, because that character separates file fields. The loader rejects corrupt rows and duplicate IDs rather than silently losing records.

`expenses.tmp` is a new snapshot and `expenses.bak` is a short-lived backup during replacement. If saving is interrupted and only the backup remains, startup asks you to restore it to `expenses.txt`. Keep your own backups for important data.

## Five-minute demonstration

1. Add ID `1`, date `2026-01-05`, category `1` (Food), amount `120`, description `Lunch`.
2. Add ID `2`, date `2026-01-06`, category `2` (Travel), amount `80`, description `Bus fare`.
3. View the records: total is `200.00`.
4. Choose Monthly summary, year `2026`, month `1`. Food is `120.00` (60%), Travel is `80.00` (40%), and average expense is `100.00`.
5. Try adding ID `1` again to show duplicate protection. Try an invalid date such as `2026-02-30` on a new record to show validation.
6. Update ID `1` to amount `150`. Search it and show the changed value.
7. Exit and restart to show that the data persists.
8. Delete ID `2`, first cancelling and then confirming.

## Viva revision

- **Why a structure?** It keeps the fields of one expense together.
- **Why an array?** A fixed array is simple for a small console project; it also gives the project a capacity limit.
- **Why functions?** Each operation has one job and can be understood separately.
- **Why file handling?** Without it, the records disappear when the program ends.
- **Why use text files?** The records can be read and inspected in a text editor.
- **How does the monthly summary work?** Filter by month, accumulate category totals, then calculate percentages and average.
- **What are the time complexities?** Search, view, summary and deletion are O(n); saving also writes all n records.
- **Why is the highest category sometimes the first one?** The first category wins a tie, as stated on the summary screen.
- **What makes it "smart"?** It summarises recorded spending by category and month; it does not use AI or predict future expenses.

## Tests

Optional automated tests need Python 3 and GCC:

```sh
python3 test_system.py
```

Tests run in temporary folders and do not alter your saved expenses. Compilation uses warnings as errors. Tests cover CRUD, persistence, monthly summaries, leap years, invalid inputs, corrupt files and failed saves.

## Scope and limitations

This is an academic single-user project, not a banking app. It has no login, encryption, cloud sync or multi-user file locking. It stores expenses only, not income or budgets. Amounts use `double` and are displayed to two decimal places; this is not an exact-decimal accounting system. Maximum amount is INR 100,000,000 per record. Dates range from 1900 to 9999. Descriptions support spaces and up to 79 bytes.

Study the code, practise the demo and follow university rules on assistance and attribution before presenting it.

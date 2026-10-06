/* Smart Expense Tracker using C
 * Author: Gurleen Kaur
 * Academic console project: structures, functions and file handling.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

#define MAX_EXPENSES 1000
#define NOTE_LENGTH 80
#define DATA_FILE "expenses.txt"
#define TEMP_FILE "expenses.tmp"
#define BACKUP_FILE "expenses.bak"
#define MAX_AMOUNT 100000000.0
#define CATEGORY_COUNT 6

const char *categories[CATEGORY_COUNT] = {
    "Food", "Travel", "Shopping", "Bills", "Education", "Other"
};

typedef struct {
    int id;
    char date[11];
    int category;
    double amount;
    char note[NOTE_LENGTH];
} Expense;

Expense expenses[MAX_EXPENSES];
int expenseCount = 0;

/* Read a whole line instead of mixing scanf with fgets.
 * Return 0 on EOF; reject overlong input rather than silently truncating it.
 */
int readLine(const char *prompt, char *buffer, size_t size) {
    int ch;
    size_t length;
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(buffer, (int)size, stdin) == NULL) return 0;
        length = strlen(buffer);
        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[--length] = '\0';
            if (length > 0 && buffer[length - 1] == '\r') buffer[--length] = '\0';
            return 1;
        }
        ch = getchar();
        if (ch == EOF || ch == '\n') return 1;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
        puts("Input is too long. Please enter a shorter value.");
    }
}

int readInteger(const char *prompt, int minimum, int maximum, int *value) {
    char line[100], *end;
    long number;
    for (;;) {
        if (!readLine(prompt, line, sizeof(line))) return 0;
        errno = 0;
        number = strtol(line, &end, 10);
        if (end == line) { puts("Enter a whole number."); continue; }
        while (isspace((unsigned char)*end)) end++;
        if (end != line && *end == '\0' && errno != ERANGE &&
            number >= minimum && number <= maximum) {
            *value = (int)number;
            return 1;
        }
        printf("Enter a whole number from %d to %d.\n", minimum, maximum);
    }
}

int readText(const char *prompt, char *value, size_t size) {
    char *start;
    size_t length;
    for (;;) {
        if (!readLine(prompt, value, size)) return 0;
        start = value;
        while (isspace((unsigned char)*start)) start++;
        memmove(value, start, strlen(start) + 1);
        length = strlen(value);
        while (length > 0 && isspace((unsigned char)value[length - 1]))
            value[--length] = '\0';
        if (length > 0 && strchr(value, '|') == NULL) return 1;
        puts("Enter non-empty text without the | character.");
    }
}

int findExpense(int id) {
    int i;
    for (i = 0; i < expenseCount; i++)
        if (expenses[i].id == id) return i;
    return -1;
}

/* Check the exact YYYY-MM-DD format and actual days in the month. */
int validDate(const char *date) {
    int i, year, month, day;
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-') return 0;
    for (i = 0; i < 10; i++)
        if (i != 4 && i != 7 && !isdigit((unsigned char)date[i])) return 0;
    year = atoi(date);
    month = atoi(date + 5);
    day = atoi(date + 8);
    if (year < 1900 || year > 9999 || month < 1 || month > 12) return 0;
    if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) days[1] = 29;
    return day >= 1 && day <= days[month - 1];
}

int readAmount(double *amount) {
    char line[100], *end;
    double number;
    for (;;) {
        if (!readLine("Amount (INR): ", line, sizeof(line))) return 0;
        errno = 0;
        number = strtod(line, &end);
        if (end == line) { puts("Enter a valid amount."); continue; }
        while (isspace((unsigned char)*end)) end++;
        if (*end == '\0' && errno != ERANGE && isfinite(number) &&
            number > 0 && number <= MAX_AMOUNT) {
            *amount = number;
            return 1;
        }
        puts("Enter a positive amount up to 100000000 (no letters or commas).");
    }
}

int readDetails(Expense *expense) {
    char line[40];
    int i;
    for (;;) {
        if (!readLine("Date (YYYY-MM-DD): ", line, sizeof(line))) return 0;
        if (validDate(line)) { strcpy(expense->date, line); break; }
        puts("Enter a real date from 1900-01-01 to 9999-12-31.");
    }
    for (i = 0; i < CATEGORY_COUNT; i++) printf("%d. %s\n", i + 1, categories[i]);
    if (!readInteger("Category number: ", 1, CATEGORY_COUNT, &expense->category)) return 0;
    expense->category--; /* Array indices start at zero. */
    return readAmount(&expense->amount) &&
           readText("Description: ", expense->note, sizeof(expense->note));
}

/* Write a new snapshot before replacing the old file. Keep a temporary
 * backup because some systems cannot rename over an existing file.
 */
int saveExpenses(void) {
    FILE *file;
    int i, hadOriginal = 0;
    file = fopen(TEMP_FILE, "w");
    if (file == NULL) { perror("Cannot create temporary file"); return 0; }
    for (i = 0; i < expenseCount; i++) {
        if (fprintf(file, "%d|%s|%d|%.17g|%s\n", expenses[i].id,
                    expenses[i].date, expenses[i].category,
                    expenses[i].amount, expenses[i].note) < 0) {
            perror("Cannot write expenses"); fclose(file); remove(TEMP_FILE); return 0;
        }
    }
    if (fclose(file) != 0) { perror("Cannot close data file"); remove(TEMP_FILE); return 0; }
    if (rename(DATA_FILE, BACKUP_FILE) == 0) hadOriginal = 1;
    else if (errno != ENOENT) { perror("Cannot back up data"); remove(TEMP_FILE); return 0; }
    if (rename(TEMP_FILE, DATA_FILE) != 0) {
        perror("Cannot save data");
        if (hadOriginal && rename(BACKUP_FILE, DATA_FILE) != 0)
            puts("Recovery needed: previous records are in expenses.bak.");
        remove(TEMP_FILE);
        return 0;
    }
    if (hadOriginal && remove(BACKUP_FILE) != 0) puts("Warning: old backup could not be removed.");
    return 1;
}

int parseRecord(char *line, Expense *expense) {
    char *fields[5], *end;
    long number;
    size_t length;
    int i;
    fields[0] = line;
    for (i = 1; i < 5; i++) {
        fields[i] = strchr(fields[i - 1], '|');
        if (fields[i] == NULL) return 0;
        *fields[i]++ = '\0';
    }
    if (strchr(fields[4], '|') != NULL) return 0;
    errno = 0;
    number = strtol(fields[0], &end, 10);
    if (end == fields[0] || *end != '\0' || errno == ERANGE || number < 1 || number > INT_MAX) return 0;
    expense->id = (int)number;
    if (!validDate(fields[1])) return 0;
    strcpy(expense->date, fields[1]);
    errno = 0;
    number = strtol(fields[2], &end, 10);
    if (end == fields[2] || *end != '\0' || errno == ERANGE || number < 0 || number >= CATEGORY_COUNT) return 0;
    expense->category = (int)number;
    errno = 0;
    expense->amount = strtod(fields[3], &end);
    if (end == fields[3] || *end != '\0' || errno == ERANGE || !isfinite(expense->amount) ||
        expense->amount <= 0 || expense->amount > MAX_AMOUNT) return 0;
    length = strlen(fields[4]);
    while (length > 0 && (fields[4][length - 1] == '\n' || fields[4][length - 1] == '\r'))
        fields[4][--length] = '\0';
    if (length == 0 || length >= NOTE_LENGTH) return 0;
    strcpy(expense->note, fields[4]);
    return 1;
}

int loadExpenses(void) {
    FILE *file;
    Expense expense;
    char line[256];
    int lineNumber = 0;
    file = fopen(DATA_FILE, "r");
    if (file == NULL) {
        if (errno == ENOENT) {
            FILE *backup = fopen(BACKUP_FILE, "r");
            if (backup != NULL) {
                fclose(backup);
                puts("Data file missing. Restore expenses.bak to expenses.txt before running again.");
                return 0;
            }
            return 1;
        }
        perror("Cannot open expenses"); return 0;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        lineNumber++;
        if (expenseCount >= MAX_EXPENSES || !parseRecord(line, &expense) || findExpense(expense.id) != -1) {
            printf("Invalid or duplicate record at line %d. Fix expenses.txt first.\n", lineNumber);
            fclose(file); return 0;
        }
        expenses[expenseCount++] = expense;
    }
    if (ferror(file)) { perror("Cannot read expenses"); fclose(file); return 0; }
    fclose(file);
    return 1;
}

void showExpense(Expense expense) {
    printf("\nID          : %d\nDate        : %s\nCategory    : %s\nAmount      : INR %.2f\nDescription : %s\n",
           expense.id, expense.date, categories[expense.category], expense.amount, expense.note);
}

void addExpense(void) {
    Expense expense;
    if (expenseCount == MAX_EXPENSES) { puts("Expense limit reached."); return; }
    if (!readInteger("Expense ID: ", 1, INT_MAX, &expense.id)) return;
    if (findExpense(expense.id) != -1) { puts("This ID already exists."); return; }
    if (!readDetails(&expense)) return;
    expenses[expenseCount++] = expense;
    if (saveExpenses()) puts("Expense added and saved.");
    else { expenseCount--; puts("Expense not added: saving failed."); }
}

void viewExpenses(void) {
    int i;
    double total = 0;
    if (expenseCount == 0) { puts("No expenses found."); return; }
    printf("\n%-10s %-10s %-12s %12s  %-28s\n", "ID", "Date", "Category", "Amount (INR)", "Description");
    puts("--------------------------------------------------------------------------------");
    for (i = 0; i < expenseCount; i++) {
        printf("%-10d %-10s %-12s %12.2f  %-28.28s\n", expenses[i].id, expenses[i].date,
               categories[expenses[i].category], expenses[i].amount, expenses[i].note);
        total += expenses[i].amount;
    }
    printf("Records: %d | Total expenses: INR %.2f\n", expenseCount, total);
    puts("Descriptions are shortened only in this table; search shows full details.");
}

void searchExpense(void) {
    int id, index;
    if (!readInteger("Expense ID to search: ", 1, INT_MAX, &id)) return;
    index = findExpense(id);
    if (index == -1) puts("Expense not found.");
    else showExpense(expenses[index]);
}

void updateExpense(void) {
    int id, index;
    Expense previous, updated;
    if (!readInteger("Expense ID to update: ", 1, INT_MAX, &id)) return;
    index = findExpense(id);
    if (index == -1) { puts("Expense not found."); return; }
    previous = expenses[index];
    updated = previous;
    showExpense(previous);
    puts("Enter replacement details. The ID stays the same.");
    if (!readDetails(&updated)) return;
    expenses[index] = updated;
    if (saveExpenses()) puts("Expense updated and saved.");
    else { expenses[index] = previous; puts("Update cancelled: saving failed."); }
}

void deleteExpense(void) {
    int id, index, i, confirm;
    Expense deleted;
    if (!readInteger("Expense ID to delete: ", 1, INT_MAX, &id)) return;
    index = findExpense(id);
    if (index == -1) { puts("Expense not found."); return; }
    showExpense(expenses[index]);
    if (!readInteger("Delete this expense? (1 = yes, 0 = cancel): ", 0, 1, &confirm) || !confirm) {
        puts("Delete cancelled."); return;
    }
    deleted = expenses[index];
    for (i = index; i < expenseCount - 1; i++) expenses[i] = expenses[i + 1];
    expenseCount--;
    if (saveExpenses()) puts("Expense deleted and saved.");
    else {
        for (i = expenseCount; i > index; i--) expenses[i] = expenses[i - 1];
        expenses[index] = deleted;
        expenseCount++;
        puts("Delete cancelled: saving failed.");
    }
}

/* Aggregate one month in a single pass through the array. */
void monthlySummary(void) {
    int year, month, i, count = 0, largest = -1;
    char prefix[8];
    double totals[CATEGORY_COUNT] = {0}, total = 0;
    if (!readInteger("Year (1900-9999): ", 1900, 9999, &year) ||
        !readInteger("Month (1-12): ", 1, 12, &month)) return;
    snprintf(prefix, sizeof(prefix), "%04d-%02d", year, month);
    for (i = 0; i < expenseCount; i++) {
        if (strncmp(expenses[i].date, prefix, 7) == 0) {
            totals[expenses[i].category] += expenses[i].amount;
            total += expenses[i].amount;
            count++;
        }
    }
    printf("\nMONTHLY SUMMARY: %s\n", prefix);
    if (count == 0) { puts("No expenses for this month."); return; }
    for (i = 0; i < CATEGORY_COUNT; i++) {
        printf("%-12s INR %12.2f (%5.1f%%)\n", categories[i], totals[i], totals[i] / total * 100);
        if (largest == -1 || totals[i] > totals[largest]) largest = i;
    }
    printf("Records: %d\nTotal: INR %.2f\nAverage per expense: INR %.2f\n",
           count, total, total / count);
    printf("Highest spending category: %s\n", categories[largest]);
    puts("If categories tie for highest spending, the first in the menu is shown.");
}

int main(void) {
    int choice;
    if (!loadExpenses()) return EXIT_FAILURE;
    puts("SMART EXPENSE TRACKER USING C");
    puts("Author: Gurleen Kaur | Academic console project");
    for (;;) {
        puts("\n1. Add expense\n2. View all expenses\n3. Search expense\n4. Update expense\n5. Delete expense\n6. Monthly summary\n0. Exit");
        if (!readInteger("Choose an option: ", 0, 6, &choice) || choice == 0) {
            puts("Goodbye. Completed changes are already saved."); break;
        }
        switch (choice) {
            case 1: addExpense(); break;
            case 2: viewExpenses(); break;
            case 3: searchExpense(); break;
            case 4: updateExpense(); break;
            case 5: deleteExpense(); break;
            case 6: monthlySummary(); break;
        }
    }
    return EXIT_SUCCESS;
}

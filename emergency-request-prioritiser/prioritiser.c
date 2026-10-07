/*
 * Emergency Request Prioritiser
 * -----------------------------
 * During a flood or landslide, help requests arrive faster than anyone can
 * answer them. This program reads the requests from a text file, gives each
 * one an urgency score using simple rules, and lists them in the order they
 * should be answered.
 *
 * The rules are simple on purpose: anyone can check why a request got its
 * score, and the program prints that reason next to every request.
 *
 * Compile:  gcc prioritiser.c -o prioritiser
 * Run:      prioritiser             (reads requests.txt)
 *           prioritiser other.txt   (reads another file)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_REQUESTS 100
#define MAX_TEXT     32
#define MAX_LINE     256
#define DEFAULT_FILE "requests.txt"

/* Scoring rules - change these numbers to change the priorities. */
#define POINTS_INJURED     5
#define POINTS_URGENT_NEED 4   /* rescue or medical       */
#define POINTS_BASIC_NEED  2   /* food or shelter         */
#define PEOPLE_PER_POINT   5   /* +1 for every 5 people   */

struct Request {
    int  id;                 /* arrival order: 1 = first request received */
    char name[MAX_TEXT];
    char village[MAX_TEXT];
    int  people;
    char need[MAX_TEXT];     /* rescue, medical, food or shelter */
    int  injured;            /* 1 = yes, 0 = no */
    int  score;
};

/* ------------------------------------------------------------------ */
/* Small text helpers                                                  */
/* ------------------------------------------------------------------ */

/* Removes spaces and line endings (\n, or \r\n on Windows) from both ends. */
void trim(char *text)
{
    int start = 0;
    int end = (int)strlen(text) - 1;

    while (text[start] != '\0' && isspace((unsigned char)text[start]))
        start++;
    while (end >= start && isspace((unsigned char)text[end]))
        end--;

    memmove(text, text + start, end - start + 1);
    text[end - start + 1] = '\0';
}

void to_lower(char *text)
{
    for (int i = 0; text[i] != '\0'; i++)
        text[i] = (char)tolower((unsigned char)text[i]);
}

/* Prints a prompt and reads one line from the keyboard.
   Returns 0 if there is no more input. */
int read_input(const char *prompt, char *buffer, int size)
{
    printf("%s", prompt);
    if (fgets(buffer, size, stdin) == NULL)
        return 0;
    trim(buffer);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Scoring                                                             */
/* ------------------------------------------------------------------ */

/* Points for the type of help needed. Returns -1 if the need is unknown. */
int need_points(const char *need)
{
    if (strcmp(need, "rescue") == 0 || strcmp(need, "medical") == 0)
        return POINTS_URGENT_NEED;
    if (strcmp(need, "food") == 0 || strcmp(need, "shelter") == 0)
        return POINTS_BASIC_NEED;
    return -1;
}

int calculate_score(const struct Request *r)
{
    int score = need_points(r->need);

    if (r->injured)
        score += POINTS_INJURED;
    score += r->people / PEOPLE_PER_POINT;
    return score;
}

/* Writes the reason for a score, e.g. "injured +5, rescue +4, 6 people +1". */
void explain_score(const struct Request *r, char *out, int size)
{
    char injured_part[24] = "";

    if (r->injured)
        snprintf(injured_part, sizeof injured_part, "injured +%d, ", POINTS_INJURED);

    snprintf(out, (size_t)size, "%s%s +%d, %d %s +%d",
             injured_part,
             r->need, need_points(r->need),
             r->people, r->people == 1 ? "person" : "people",
             r->people / PEOPLE_PER_POINT);
}

/* ------------------------------------------------------------------ */
/* Reading requests                                                    */
/* ------------------------------------------------------------------ */

/*
 * Turns one line like "Ramesh,Kullu,6,rescue,yes" into a Request.
 * Returns 1 if the line is valid. If not, writes the reason into `error`
 * and returns 0, so a bad line is reported instead of silently ignored.
 */
int parse_request(char *line, struct Request *r, char *error, int error_size)
{
    char *parts[5];
    int count = 0;
    char *piece = strtok(line, ",");

    while (piece != NULL && count < 5) {
        parts[count++] = piece;
        piece = strtok(NULL, ",");
    }
    if (count != 5 || piece != NULL) {
        snprintf(error, (size_t)error_size, "expected 5 values separated by commas");
        return 0;
    }
    for (int i = 0; i < 5; i++)
        trim(parts[i]);

    /* name and village */
    if (strlen(parts[0]) == 0 || strlen(parts[1]) == 0) {
        snprintf(error, (size_t)error_size, "name and village cannot be empty");
        return 0;
    }
    if (strlen(parts[0]) >= MAX_TEXT || strlen(parts[1]) >= MAX_TEXT) {
        snprintf(error, (size_t)error_size, "name or village is longer than %d letters", MAX_TEXT - 1);
        return 0;
    }

    /* number of people */
    char *end;
    long people = strtol(parts[2], &end, 10);
    if (parts[2][0] == '\0' || *end != '\0' || people < 1 || people > 10000) {
        snprintf(error, (size_t)error_size, "number of people must be a whole number from 1 to 10000");
        return 0;
    }

    /* type of need */
    to_lower(parts[3]);
    if (need_points(parts[3]) < 0) {
        snprintf(error, (size_t)error_size, "need must be rescue, medical, food or shelter (got \"%s\")", parts[3]);
        return 0;
    }

    /* injured? */
    to_lower(parts[4]);
    if (strcmp(parts[4], "yes") == 0) {
        r->injured = 1;
    } else if (strcmp(parts[4], "no") == 0) {
        r->injured = 0;
    } else {
        snprintf(error, (size_t)error_size, "injured must be yes or no");
        return 0;
    }

    strcpy(r->name, parts[0]);
    strcpy(r->village, parts[1]);
    r->people = (int)people;
    strcpy(r->need, parts[3]);
    r->score = calculate_score(r);
    return 1;
}

/* Reads every request in the file. Returns how many lines were skipped. */
int load_requests(const char *filename, struct Request list[], int *count)
{
    FILE *file = fopen(filename, "r");
    char line[MAX_LINE];
    char error[120];
    int line_number = 0;
    int skipped = 0;

    *count = 0;
    if (file == NULL) {
        printf("Could not open %s - starting with an empty list.\n", filename);
        return 0;
    }

    while (fgets(line, sizeof line, file) != NULL) {
        struct Request r;

        line_number++;
        trim(line);
        if (line[0] == '\0' || line[0] == '#')
            continue;   /* blank line or comment */

        if (*count >= MAX_REQUESTS) {
            printf("Skipped line %d: the list is full (%d requests max)\n", line_number, MAX_REQUESTS);
            skipped++;
        } else if (parse_request(line, &r, error, sizeof error)) {
            r.id = *count + 1;
            list[*count] = r;
            (*count)++;
        } else {
            printf("Skipped line %d: %s\n", line_number, error);
            skipped++;
        }
    }
    fclose(file);
    return skipped;
}

/* ------------------------------------------------------------------ */
/* Sorting and showing                                                 */
/* ------------------------------------------------------------------ */

/*
 * Bubble sort, highest score first. We only swap when the next score is
 * strictly higher, so when two scores are equal the request that arrived
 * first stays ahead.
 */
void sort_by_priority(struct Request list[], int count)
{
    for (int pass = 0; pass < count - 1; pass++) {
        for (int i = 0; i < count - 1 - pass; i++) {
            if (list[i + 1].score > list[i].score) {
                struct Request temp = list[i];
                list[i] = list[i + 1];
                list[i + 1] = temp;
            }
        }
    }
}

void show_queue(const struct Request list[], int count)
{
    struct Request sorted[MAX_REQUESTS];
    char why[80];

    if (count == 0) {
        printf("\nNo requests yet.\n");
        return;
    }

    /* Sort a copy, so the original list stays in arrival order. */
    for (int i = 0; i < count; i++)
        sorted[i] = list[i];
    sort_by_priority(sorted, count);

    printf("\nPRIORITY QUEUE - answer from the top\n");
    printf("%-4s %-5s %-10s %-10s %6s  %-8s %s\n",
           "Rank", "Score", "Name", "Village", "People", "Need", "Why this score");
    printf("----------------------------------------------------------------------------\n");
    for (int i = 0; i < count; i++) {
        explain_score(&sorted[i], why, sizeof why);
        printf("#%-3d %-5d %-10s %-10s %6d  %-8s %s\n",
               i + 1, sorted[i].score, sorted[i].name, sorted[i].village,
               sorted[i].people, sorted[i].need, why);
    }
}

/* Totals per village, so a team can see which area needs the most help. */
void show_village_summary(const struct Request list[], int count)
{
    char villages[MAX_REQUESTS][MAX_TEXT];
    int requests[MAX_REQUESTS] = {0};
    int people[MAX_REQUESTS] = {0};
    int injured[MAX_REQUESTS] = {0};
    int village_count = 0;
    int total_people = 0;
    int total_injured = 0;

    if (count == 0) {
        printf("\nNo requests yet.\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        int found = -1;

        for (int v = 0; v < village_count; v++) {
            if (strcmp(villages[v], list[i].village) == 0) {
                found = v;
                break;
            }
        }
        if (found == -1) {              /* first request from this village */
            found = village_count;
            strcpy(villages[found], list[i].village);
            village_count++;
        }
        requests[found]++;
        people[found] += list[i].people;
        injured[found] += list[i].injured;
        total_people += list[i].people;
        total_injured += list[i].injured;
    }

    printf("\nSUMMARY BY VILLAGE\n");
    printf("%-12s %8s %8s %16s\n", "Village", "Requests", "People", "With injuries");
    printf("-----------------------------------------------\n");
    for (int v = 0; v < village_count; v++)
        printf("%-12s %8d %8d %16d\n", villages[v], requests[v], people[v], injured[v]);
    printf("-----------------------------------------------\n");
    printf("%-12s %8d %8d %16d\n", "Total", count, total_people, total_injured);
}

/* ------------------------------------------------------------------ */
/* Adding and saving                                                   */
/* ------------------------------------------------------------------ */

void add_request(struct Request list[], int *count)
{
    char name[MAX_LINE], village[MAX_LINE], people[MAX_LINE], need[MAX_LINE], injured[MAX_LINE];
    char line[MAX_LINE * 5 + 8];
    char error[120];
    struct Request r;

    if (*count >= MAX_REQUESTS) {
        printf("The list is full (%d requests max).\n", MAX_REQUESTS);
        return;
    }

    printf("\nNEW REQUEST\n");
    if (!read_input("Name: ", name, sizeof name) ||
        !read_input("Village: ", village, sizeof village) ||
        !read_input("Number of people: ", people, sizeof people) ||
        !read_input("Need (rescue/medical/food/shelter): ", need, sizeof need) ||
        !read_input("Anyone injured? (yes/no): ", injured, sizeof injured))
        return;

    /* Build a line in the file format and reuse the same checks. */
    snprintf(line, sizeof line, "%s,%s,%s,%s,%s", name, village, people, need, injured);
    if (parse_request(line, &r, error, sizeof error)) {
        r.id = *count + 1;
        list[*count] = r;
        (*count)++;
        printf("Added %s from %s with score %d.\n", r.name, r.village, r.score);
    } else {
        printf("Not added: %s\n", error);
    }
}

int save_requests(const char *filename, const struct Request list[], int count)
{
    FILE *file = fopen(filename, "w");

    if (file == NULL) {
        printf("Could not save to %s\n", filename);
        return 0;
    }
    fprintf(file, "# Emergency requests - one per line, in the order they arrived\n");
    fprintf(file, "# Format: name,village,people,need,injured\n");
    for (int i = 0; i < count; i++)
        fprintf(file, "%s,%s,%d,%s,%s\n", list[i].name, list[i].village,
                list[i].people, list[i].need, list[i].injured ? "yes" : "no");
    fclose(file);
    printf("Saved %d requests to %s\n", count, filename);
    return 1;
}

/* ------------------------------------------------------------------ */
/* Menu                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    struct Request list[MAX_REQUESTS];
    int count = 0;
    const char *filename = (argc > 1) ? argv[1] : DEFAULT_FILE;
    char choice[MAX_LINE];

    printf("=== Emergency Request Prioritiser ===\n");
    int skipped = load_requests(filename, list, &count);
    printf("Loaded %d request%s from %s", count, count == 1 ? "" : "s", filename);
    if (skipped > 0)
        printf(" (%d line%s skipped - see above)", skipped, skipped == 1 ? "" : "s");
    printf("\n");

    while (1) {
        printf("\n1. Show priority queue\n");
        printf("2. Add a request\n");
        printf("3. Summary by village\n");
        printf("4. Save and exit\n");
        printf("5. Exit without saving\n");
        if (!read_input("Choose 1-5: ", choice, sizeof choice))
            break;   /* no more input */

        if (strcmp(choice, "1") == 0) {
            show_queue(list, count);
        } else if (strcmp(choice, "2") == 0) {
            add_request(list, &count);
        } else if (strcmp(choice, "3") == 0) {
            show_village_summary(list, count);
        } else if (strcmp(choice, "4") == 0) {
            if (skipped > 0) {
                char answer[MAX_LINE];
                printf("%d line%s skipped when loading, and saving will not keep %s.\n",
                       skipped, skipped == 1 ? " was" : "s were", skipped == 1 ? "it" : "them");
                if (!read_input("Save anyway? (y/n): ", answer, sizeof answer) ||
                    (answer[0] != 'y' && answer[0] != 'Y')) {
                    printf("Not saved.\n");
                    continue;
                }
            }
            save_requests(filename, list, count);
            break;
        } else if (strcmp(choice, "5") == 0) {
            printf("Exited without saving.\n");
            break;
        } else {
            printf("Please type a number from 1 to 5.\n");
        }
    }
    return 0;
}

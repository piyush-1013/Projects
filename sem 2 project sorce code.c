
//Student Mental Health Simulator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SAVE_FILE     "student_data.txt"
#define SAVE_VERSION  2

/* ── helpers ─────────────────────────────────────────────────────────── */

// Clamps the given value 'val' to be within the range [min, max].

int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

char grade(int avg) {
    if (avg >= 90) return 'A';
    if (avg >= 75) return 'B';
    if (avg >= 55) return 'C';
    if (avg >= 35) return 'D';
    return 'F';
}

int read_int(const char *prompt) {
    int val;
    while (1) {
        if (prompt) printf("%s", prompt);
        if (scanf("%d", &val) == 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            return val;
        }
        int c; while ((c = getchar()) != '\n' && c != EOF);
        printf(" Invalid input. Please enter a number.\n");
    }
}

/* ── one day's action ────────────────────────────────────────────────── */

/* FIX 1: Removed unused 'day' parameter from play_session signature */
int play_session(int *energy, int *happiness, int *stress,
                 int last_action, int *out_choice,
                 int day_bonus_prod, int day_stress_extra) {

    /* burnout / exhaustion check */
    if (*stress >= 100 || *energy <= 0) {
        if (*stress >= 100) {
            printf("\n [BURNOUT] Stress maxed — Forced rest.\n");
            *energy += 20; *stress -= 15;
        } else {
            printf("\n [EXHAUSTED] Passed out! Turn skipped.\n");
            *energy += 50; *stress -= 10;
        }
        *energy = clamp(*energy, 0, 100);
        *stress = clamp(*stress, 0, 100);
        *out_choice = 0;
        return 0;
    }

    /* menu */
    printf("\nWhat do you do today?\n");
    printf("1. Study    (Prod +%d, Energy -20, Stress +%d)\n", 20 + day_bonus_prod, 10 + day_stress_extra);
    printf("2. Reels    (Happy +8,  Energy -10, Stress -5)\n");
    printf("3. Game     (Happy +15, Energy -15, Stress -10)\n");
    printf("4. Sleep    (Energy +30, Stress -8, Prod -5)\n");
    printf("5. Hangout  (Happy +20, Energy -20, Prod -8, Stress +5)\n");
    printf("6. Exit App (No Save)\n");

    int choice;
    while (1) {
        choice = read_int("Choice: ");
        if (choice != 6) break;
        if (read_int(" Exit without saving? (1. Yes  2. No): ") == 1) {
            printf("\n Exiting...\n");
            exit(0);
        }
        printf(" Cancelled. Pick again.\n");
    }

    if (choice < 1 || choice > 5) {
        printf(" Invalid choice. Turn wasted.\n");
        *out_choice = 0;
        return 0;
    }

    /* stat deltas table: dp, de, dh, ds */
    int deltas[5][4] = {
        { 20, -20,  -5,  10 },   /* 1. Study   */
        {  0, -10,   8,  -5 },   /* 2. Reels   */
        {  0, -15,  15, -10 },   /* 3. Game    */
        { -5,  30,   0,  -8 },   /* 4. Sleep   */
        { -8, -20,  20,   5 },   /* 5. Hangout */
    };
    int dp = deltas[choice-1][0], de = deltas[choice-1][1],
        dh = deltas[choice-1][2], ds = deltas[choice-1][3];
    if (choice == 1) { dp += day_bonus_prod; ds += day_stress_extra; }

    if (choice == 1 && *happiness <= 0) {
        dp /= 2;
        printf(" [Low Happiness Penalty] Productivity gain halved today.\n");
    }

    int daily_prod = clamp(dp,            0, 100);
    *energy        = clamp(*energy  + de, 0, 100);
    *happiness     = clamp(*happiness + dh, 0, 100);
    *stress        = clamp(*stress   + ds, 0, 100);

    int streak_bonus = 0;
    if (choice == 1 && last_action == 1) {
        streak_bonus = 10;
        daily_prod = clamp(daily_prod + streak_bonus, 0, 100);
    }

    printf("\n  Result: ");
    if (dp != 0 || streak_bonus != 0) printf("Productivity %+d  ", dp + streak_bonus);
    printf("Energy %+d  Happiness %+d  Stress %+d\n", de, dh, ds);
    if (streak_bonus > 0) printf("  (includes Study Streak bonus: +%d)\n", streak_bonus);

    *out_choice = choice;
    return daily_prod;
}

/* ── main ─────────────────────────────────────────────────────────────── */

int main(void) {
    clear_screen();

    /* action names — declared once, reused everywhere */
    const char *anames[] = {"(skipped)", "Studied", "Watched Reels",
                             "Played Games", "Slept Early", "Hung Out"};

    char current_name[50], saved_name[50];
    int day = 1, energy = 80, happiness = 50, stress = 10;
    int hist_prod[5]   = {0, 0, 0, 0, 0};
    int hist_action[5] = {0, 0, 0, 0, 0};
    int last_action = 0, cooldown_mode = 0;
    time_t last_played_time = 0;
    /* FIX 2: current_time declared here so it is accessible in STEP 5 */
    time_t current_time;
    FILE *file;
    srand((unsigned)time(NULL));

    /* ── STEP 1: authentication ── */
    printf("=== STUDENT LIFE SIMULATOR (Mobile Ed.) ===\n");
    printf(" Who is playing? Enter Name: ");
    scanf("%49s", current_name);
    { int c; while ((c = getchar()) != '\n' && c != EOF); }

    file = fopen(SAVE_FILE, "r");
    if (file == NULL) {
        printf("\n New System Detected. Welcome, %s!\n", current_name);
        cooldown_mode = read_int(">> Select Mode:\n 1. Speedrun (No Wait)\n 2. Realism  (24h Cooldown)\nChoice: ");
    } else {
        int saved_version = 0;
        fscanf(file, "%d", &saved_version);
        if (saved_version != SAVE_VERSION) {
            fclose(file);
            remove(SAVE_FILE);
            printf("\n Save file is from an older version. Starting fresh.\n");
            cooldown_mode = read_int(">> Select Mode:\n 1. Speedrun (No Wait)\n 2. Realism  (24h Cooldown)\nChoice: ");
        } else if (fscanf(file, "%49s", saved_name) == 1) {
            if (strcmp(current_name, saved_name) == 0) {
                /* FIX 3: cast last_played_time read target to (long*) to match %ld */
                fscanf(file, "%d %d %d %d %d %ld %d",
                       &day, &energy, &happiness, &stress,
                       &last_action, (long *)&last_played_time, &cooldown_mode);
                for (int i = 0; i < 5; i++) fscanf(file, "%d", &hist_prod[i]);
                for (int i = 0; i < 5; i++) fscanf(file, "%d", &hist_action[i]);
                fclose(file);

                double seconds_passed = difftime(time(NULL), last_played_time);
                if (cooldown_mode == 2 && seconds_passed < 86400.0) {
                    printf("\n STOP! You need to rest.\n");
                    printf("    Time remaining: %.1f hours.\n",
                           (86400.0 - seconds_passed) / 3600.0);
                    if (read_int("\n 1. Wait and Exit\n 2. Skip Time\nChoice: ") == 1)
                        return 0;
                    printf(" Time Skipped! Loading next day...\n");
                }

                printf("\n Identity Verified. Welcome back, %s! Loading Day %d\n",
                       current_name, day);
                if (day > 1) {
                    int safe_act = (last_action >= 0 && last_action <= 5) ? last_action : 0;
                    printf(" Yesterday (Day %d): You %s. Productivity was %d.\n",
                           day - 1, anames[safe_act], hist_prod[day - 2]);
                }

                printf(" Current Mode: %s | Change? 1. Keep  2. Speedrun  3. Realism\n",
                       cooldown_mode == 2 ? "Realism" : "Speedrun");
                int m = read_int("Choice: ");
                if (m == 2) cooldown_mode = 1;
                else if (m == 3) cooldown_mode = 2;

            } else {
                printf("\n ALERT: Save belongs to '%s', but you are '%s'.\n",
                       saved_name, current_name);
                fclose(file);
                if (read_int("  1. Yes, Start Fresh\n  2. No, Exit\nChoice: ") == 1) {
                    day = 1; energy = 80; happiness = 50; stress = 10;
                    remove(SAVE_FILE);
                    printf("\n Old save deleted. Starting Day 1 for %s\n", current_name);
                    cooldown_mode = read_int(" Select Mode:\n 1. Speedrun\n 2. Realism\nChoice: ");
                } else {
                    return 0;
                }
            }
        } else {
            fclose(file);
            printf("Error reading save file. Starting fresh.\n");
        }
    }

    /* ── STEP 2: final report (game already finished) ── */
    if (day > 5) {
        clear_screen();
        printf("=== Final Report Card ===\n\nDay-by-Day Recap\n");

        int max_streak = 0, cur_streak = 0, total_prod = 0;
        for (int i = 0; i < 5; i++) {
            int act = (hist_action[i] >= 0 && hist_action[i] <= 5) ? hist_action[i] : 0;
            printf("Day %d: %-15s | ", i + 1, anames[act]);
            for (int j = 0; j < hist_prod[i] / 10; j++) printf("#");
            printf(" (%d)\n", hist_prod[i]);
            total_prod += hist_prod[i];
            if (hist_action[i] == 1) {
                if (++cur_streak > max_streak) max_streak = cur_streak;
            } else {
                cur_streak = 0;
            }
        }

        int avg_prod = total_prod / 5;
        int regret   = (100 - avg_prod) + stress - (happiness / 2);
        if (regret < 0) regret = 0;
        char g = grade(avg_prod);

        if      (max_streak >= 3) printf("\n  Study Streak: %d days in a row! Impressive.\n", max_streak);
        else if (max_streak == 2) printf("\n  Study Streak: 2-day streak spotted.\n");

        printf("\n=== Final Summary ===\n");
        printf("  Avg Productivity : %d\n", avg_prod);
        printf("  Final Stress     : %d  (lower is better)\n", stress);
        printf("  Final Happiness  : %d  (higher is better)\n", happiness);
        printf("  Regret Score     : %d  = (100 - avg) + stress - (happiness/2)\n", regret);
        printf("\n  Final Grade: %c  ", g);

        if      (avg_prod >= 80 && stress > 60)   printf("| Identity: Burnt Out\n");
        else if (avg_prod >= 80)                   printf("| Identity: Academic Weapon\n");
        else if (g == 'A' || g == 'B')             printf("| Identity: Quiet Grinder\n");
        else if (happiness > 70 && avg_prod < 40)  printf("| Identity: Happy-Go-Lucky\n");
        else if (stress > 70 && avg_prod < 50)     printf("| Identity: Eternal Slacker in Panic\n");
        else if (regret >= 70)                     printf("| Identity: Panic Master\n");
        else                                       printf("| Identity: Survivor\n");

        printf("  [Target: avg productivity >= 75 = Grade B or above]\n");
        if (read_int("\n>> 1. Play Again\n   2. Exit\nChoice: ") == 1) {
            remove(SAVE_FILE);
            printf("\n Save wiped! You got a %c this run. Can you do better?\n", g);
        }
        return 0;
    }

    /* ── STEP 3: day intro ── */
    if (day == 1) {
        printf("\n First day. You're fresh. Energy: %d\n", energy);
    } else {
        energy = clamp(energy + 15, 0, 100);
        printf("\n You got some rest. Energy +15 (now %d)\n", energy);
    }

    /* day modifiers — lookup tables */
    int bonus_prod[]   = {0,  0,  0,  5, 10};
    int stress_extra[] = {0,  5,  5, 10, 15};
    const char *effects[] = {
        " Effect: Normal day. No modifiers.\n",
        " Effect: Tension in the air. Studying costs +5 extra stress.\n",
        " Effect: Midweek slump. Studying costs +5 extra stress.\n",
        " Effect: Deadline nearing! Study gives +5 bonus productivity but +10 stress.\n",
        " Effect: DEADLINE DAY! Study gives +10 bonus but +15 stress.\n",
    };
    const char *themes[] = {
        "Monday Blues", "Tuesday Tension",
        "Midweek Crisis", "Panic Rising", "DEADLINE PANIC!"
    };

    int day_bonus_prod   = bonus_prod[day - 1];
    int day_stress_extra = stress_extra[day - 1];

    printf("\n=== DAY %d ===\n", day);
    printf("Theme: %s\n", themes[day - 1]);
    printf("%s", effects[day - 1]);
    printf(" Goal: avg productivity >= 75 across all 5 days = Grade B or higher\n");

    printf("\nMood: ");
    if      (stress > 60) printf("STRESSED | ");
    else if (energy < 20) printf("TIRED | ");
    else                  printf("HAPPY | ");
    printf("Energy: %d | Happiness: %d | Stress: %d\n", energy, happiness, stress);

    if (last_action == 1)
        printf(" Study Streak: ACTIVE! Study again today for +10 bonus productivity.\n");

    printf("\n[ SYSTEM ANALYSIS ]\n");
    if      (stress > 60 && happiness < 30) printf(" Status: Burnout Imminent.\n");
    else if (energy > 70 && stress < 20)    printf(" Status: Too Relaxed. Zero urgency.\n");
    else if (happiness > 80)                printf(" Status: Vibing. Grades might suffer.\n");
    else                                    printf(" Status: Balanced. Keep it up.\n");

    /* ── STEP 4: play the day ── */
    /* FIX 4: These steps were placed OUTSIDE main's closing brace in the original.
       Moved inside main, and updated call to match the fixed play_session signature. */
    int choice = 0;
    int daily_prod = play_session(&energy, &happiness, &stress,
                                  last_action, &choice,
                                  day_bonus_prod, day_stress_extra);

    /* random event (33% chance) */
    if (choice != 0 && (rand() % 3) == 0) {
        const char *event_msgs[] = {
            " A friend cancelled plans. Happiness -5\n",
            " Surprisingly good sleep last night. Energy +10\n",
            " Professor announced a surprise assignment. Stress +10\n",
            " Your friend sent a meme. Happiness +10\n",
        };
        int  ev_delta[] = { -5,  10,  10,  10 };
        int *ev_stat[]  = { &happiness, &energy, &stress, &happiness };
        int which = rand() % 4;
        *ev_stat[which] = clamp(*ev_stat[which] + ev_delta[which], 0, 100);
        printf("\n%s", event_msgs[which]);
    }

    /* final clamp + record */
    energy     = clamp(energy,     0, 100);
    happiness  = clamp(happiness,  0, 100);
    stress     = clamp(stress,     0, 100);
    daily_prod = clamp(daily_prod, 0, 100);
    hist_prod[day - 1]   = daily_prod;
    hist_action[day - 1] = choice;
    last_action = choice;
    day++;

    /* ── STEP 5: save ── */
    file = fopen(SAVE_FILE, "w");
    if (file != NULL) {
        current_time = time(NULL);
        fprintf(file, "%d\n", SAVE_VERSION);
        fprintf(file, "%s %d %d %d %d %d %ld %d\n",
                current_name, day, energy, happiness, stress,
                last_action, (long)current_time, cooldown_mode);
        for (int i = 0; i < 5; i++) fprintf(file, "%d ", hist_prod[i]);
        fprintf(file, "\n");
        for (int i = 0; i < 5; i++) fprintf(file, "%d ", hist_action[i]);
        fclose(file);
        if (day > 5)
            printf("\n Progress Saved! Run the game again to see your Final Report Card.\n");
        else
            printf("\n Progress Saved! Come back for Day %d.\n", day);
    } else {
        printf("\n Error saving progress!\n");
    }

    /* FIX 5: Replaced the erroneous ] at the end with the correct } to close main */
    return 0;
}


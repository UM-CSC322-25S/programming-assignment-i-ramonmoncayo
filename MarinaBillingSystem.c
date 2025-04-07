/**
 * MarinaBillingSystem.c
 *
 * A simple program for the Marina Manager of Nautical Ventures, tracking boats,
 * their locations, amounts owed, and monthly fees. Data is read from a CSV file
 * and written back upon exit.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* --------------------------------------------------------------------------
   Constant definitions
   -------------------------------------------------------------------------- */
#define MAX_BOATS      120
#define MAX_NAME       128
#define MONTH_SLIP     12.50
#define MONTH_LAND     14.00
#define MONTH_TRAILOR  25.00
#define MONTH_STORAGE  11.20

/* --------------------------------------------------------------------------
   Type definitions
   -------------------------------------------------------------------------- */

/**
 * LocationType - an enumeration identifying where a boat is kept (slip, land,
 * trailor, or storage).
 */
typedef enum {
    SLIP,
    LAND,
    TRAILOR,
    STORAGE
} LocationType;


/**
 * LocationDetail - a union that stores the extra piece of data about each boat
 * depending on the LocationType:
 *   - SLIP     => slipNumber   (int)
 *   - LAND     => bayLetter    (char)
 *   - TRAILOR  => licenseTag   (char[32])
 *   - STORAGE  => storageNum   (int)
 */
typedef union {
    int  slipNumber;       
    char bayLetter;        
    char licenseTag[32];   
    int  storageNum;       
} LocationDetail;

/**
 * Boat - a struct representing a single boat, including:
 *   - name (up to 127 characters, excluding commas)
 *   - length (0..100)
 *   - locType (SLIP, LAND, TRAILOR, STORAGE)
 *   - detail (the union above)
 *   - amountOwed (how much this boat owes the marina)
 */
typedef struct {
    char           name[MAX_NAME];
    int            length;      
    LocationType   locType;
    LocationDetail detail;
    double         amountOwed;
} Boat;


/**
 * BoatManager - a struct to hold an array of up to MAX_BOATS pointers to Boat,
 * and a count of how many are in use.  We pass around a pointer to this manager
 * so that there are no global variables.
 */
typedef struct {
    Boat* boats[MAX_BOATS];
    int   numBoats; 
} BoatManager;


/* --------------------------------------------------------------------------
   Function Prototypes
   -------------------------------------------------------------------------- */

/**
 * parseLocationType
 *    Convert a location-type string (e.g. "slip", "land", "trailor", "storage")
 *    to the corresponding enum constant.
 */
LocationType parseLocationType(const char *locStr);

/**
 * caseInsensitiveCompare
 *    Compare two strings ignoring case (returns negative if a<b, 0 if equal,
 *    positive if a>b).
 */
int caseInsensitiveCompare(const char *a, const char *b);

/**
 * locationTypeString
 *    Return the string name for a given LocationType enum.
 */
const char* locationTypeString(LocationType t);

/**
 * loadFromCSV
 *    Load boat data from a CSV file (filename). On success, the data is placed
 *    into the manager. If the file does not exist, manager remains empty.
 */
void loadFromCSV(BoatManager *manager, const char *filename);

/**
 * saveToCSV
 *    Save boat data from manager into a CSV file (overwrites existing file).
 */
void saveToCSV(const BoatManager *manager, const char *filename);

/**
 * printInventory
 *    Print a sorted list (alphabetical by boat name) of all boats in manager.
 */
void printInventory(const BoatManager *manager);

/**
 * addBoat
 *    Prompt the user for CSV-like input, create a new Boat, store in manager,
 *    re-sort by name.
 */
void addBoat(BoatManager *manager);

/**
 * removeBoat
 *    Prompt for a boat name; if found, remove from manager. Otherwise show error.
 */
void removeBoat(BoatManager *manager);

/**
 * acceptPayment
 *    Prompt for boat name and payment amount. Subtract from the boat's owed
 *    balance if valid.
 */
void acceptPayment(BoatManager *manager);

/**
 * monthlyUpdate
 *    Apply monthly charges to each boat's owed amount, depending on location.
 */
void monthlyUpdate(BoatManager *manager);

/**
 * findBoatIndex
 *    Return the index of the boat (case-insensitive name match), or -1 if not found.
 */
int findBoatIndex(const BoatManager *manager, const char *name);

/**
 * createBoat
 *    Allocate a new Boat struct, initialize from given data, and return pointer.
 */
Boat* createBoat(const char *name, int length, LocationType locType,
                 const char *detailStr, double owed);

/**
 * freeAllBoats
 *    Helper that frees all boats in manager, then sets count to zero.
 */
void freeAllBoats(BoatManager *manager);

/**
 * sortBoatsByName
 *    Sort the array of boat pointers in manager by boat name (case-insensitive).
 *    Uses qsort().
 */
void sortBoatsByName(BoatManager *manager);

/* --------------------------------------------------------------------------
   Implementation
   -------------------------------------------------------------------------- */

LocationType parseLocationType(const char *locStr)
{
    /* Convert to lower case to compare more easily */
    char lower[32];
    int i;
    for (i = 0; locStr[i] && i < 31; i++) {
        lower[i] = (char)tolower((unsigned char)locStr[i]);
    }
    lower[i] = '\0';

    if (strcmp(lower, "slip") == 0)       return SLIP;
    if (strcmp(lower, "land") == 0)       return LAND;
    if (strcmp(lower, "trailor") == 0)    return TRAILOR;
    if (strcmp(lower, "storage") == 0)    return STORAGE;

    /* Default/fallback if unrecognized input */
    return SLIP;
}


int caseInsensitiveCompare(const char *a, const char *b)
{
    /* Compare character by character ignoring case */
    for (;;) {
        unsigned char ca = (unsigned char)tolower((unsigned char)*a);
        unsigned char cb = (unsigned char)tolower((unsigned char)*b);
        if (ca < cb) return -1;
        if (ca > cb) return  1;
        if (ca == 0) return  0;  /* both strings ended */
        a++;
        b++;
    }
}


const char* locationTypeString(LocationType t)
{
    switch (t) {
        case SLIP:     return "slip";
        case LAND:     return "land";
        case TRAILOR:  return "trailor";
        case STORAGE:  return "storage";
        default:       return "slip";
    }
}


static int boatCompareByName(const void *p1, const void *p2)
{
    /* qsort comparator must take two 'const void*' pointers, each to a Boat* */
    Boat * const *b1 = (Boat **)p1;
    Boat * const *b2 = (Boat **)p2;
    if (*b1 == NULL && *b2 == NULL) return 0;
    if (*b1 == NULL) return 1;   /* push NULLs to the end, if any */
    if (*b2 == NULL) return -1;
    return caseInsensitiveCompare((*b1)->name, (*b2)->name);
}

void sortBoatsByName(BoatManager *manager)
{
    /* Only sort the active portion (manager->numBoats). */
    qsort(manager->boats, manager->numBoats, sizeof(Boat*), boatCompareByName);
}


Boat* createBoat(const char *name, int length, LocationType locType,
                 const char *detailStr, double owed)
{
    Boat *b = (Boat *)malloc(sizeof(Boat));
    if (!b) {
        /* Caller can handle error message or fallback if needed. */
        return NULL;
    }
    /* Initialize the fields carefully */
    strncpy(b->name, name, MAX_NAME - 1);
    b->name[MAX_NAME - 1] = '\0';

    b->length     = length;
    b->locType    = locType;
    b->amountOwed = owed;

    /* Initialize the union depending on location type */
    switch (locType) {
        case SLIP:
            b->detail.slipNumber = atoi(detailStr);
            break;
        case LAND:
            b->detail.bayLetter = detailStr[0];  /* e.g. 'C' */
            break;
        case TRAILOR:
            /* copy the tag up to 31 chars */
            strncpy(b->detail.licenseTag, detailStr, 31);
            b->detail.licenseTag[31] = '\0';
            break;
        case STORAGE:
            b->detail.storageNum = atoi(detailStr);
            break;
    }
    return b;
}


void loadFromCSV(BoatManager *manager, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        /* If file does not exist or can't open, we just return an empty manager */
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        /*
         * Each line: boatName,length,locType,detail,owed
         * Example: "Big Brother,20,slip,27,1450.00"
         *
         * We must parse carefully (boat name might contain spaces but not commas).
         * We can safely do something like:
         *   sscanf(line, " %[^,],%d,%[^,],%[^,],%lf", ...)
         */
        char boatName[128], locStr[32], detailStr[64];
        int length;
        double owed;

        if (5 == sscanf(line, " %[^,],%d,%[^,],%[^,],%lf",
                        boatName, &length, locStr, detailStr, &owed)) 
        {
            /* Create new boat struct */
            Boat *b = createBoat(boatName, length, parseLocationType(locStr),
                                 detailStr, owed);
            if (b) {
                /* Insert into manager if there's room. */
                if (manager->numBoats < MAX_BOATS) {
                    manager->boats[manager->numBoats] = b;
                    manager->numBoats++;
                } else {
                    /* If we are full, discard the boat (or handle differently). */
                    free(b);
                }
            }
        }
        /* else if parse fails, we skip that line. */
    }

    fclose(fp);
    /* Finally, ensure sorted after loading. */
    sortBoatsByName(manager);
}


void saveToCSV(const BoatManager *manager, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Unable to write file '%s'\n", filename);
        return;
    }

    /* Write each boat in CSV format. */
    for (int i = 0; i < manager->numBoats; i++) {
        Boat *b = manager->boats[i];
        if (!b) continue;  /* safety check */

        switch (b->locType) {
            case SLIP:
                fprintf(fp, "%s,%d,slip,%d,%.2f\n",
                        b->name, b->length, b->detail.slipNumber, b->amountOwed);
                break;
            case LAND:
                fprintf(fp, "%s,%d,land,%c,%.2f\n",
                        b->name, b->length, b->detail.bayLetter, b->amountOwed);
                break;
            case TRAILOR:
                fprintf(fp, "%s,%d,trailor,%s,%.2f\n",
                        b->name, b->length, b->detail.licenseTag, b->amountOwed);
                break;
            case STORAGE:
                fprintf(fp, "%s,%d,storage,%d,%.2f\n",
                        b->name, b->length, b->detail.storageNum, b->amountOwed);
                break;
        }
    }

    fclose(fp);
}


void printInventory(const BoatManager *manager)
{
    /*
     * Example line format from the spec:
     *   Big Brother           20'    slip   # 27   Owes $1200.00
     *
     * We'll use a carefully aligned printf approach.
     */
    for (int i = 0; i < manager->numBoats; i++) {
        const Boat *b = manager->boats[i];
        if (!b) continue;  /* skip nulls if any exist */

        /* Print the boat name left-justified in ~22 spaces. */
        printf("%-22s %2d' ", b->name, b->length);

        switch (b->locType) {
            case SLIP:
                printf("   slip   # %2d   Owes $%7.2f\n",
                       b->detail.slipNumber, b->amountOwed);
                break;
            case LAND:
                printf("   land      %c   Owes $%7.2f\n",
                       b->detail.bayLetter, b->amountOwed);
                break;
            case TRAILOR:
                printf("trailor %6s   Owes $%7.2f\n",
                       b->detail.licenseTag, b->amountOwed);
                break;
            case STORAGE:
                printf("storage   # %2d   Owes $%7.2f\n",
                       b->detail.storageNum, b->amountOwed);
                break;
        }
    }
    printf("\n");
}


int findBoatIndex(const BoatManager *manager, const char *name)
{
    /* Linear search for a boat with a matching name (case-insensitive). */
    for (int i = 0; i < manager->numBoats; i++) {
        if (caseInsensitiveCompare(manager->boats[i]->name, name) == 0) {
            return i;
        }
    }
    return -1;
}


void addBoat(BoatManager *manager)
{
    /* Prompt for CSV-like line, e.g. "Brooks,34,trailor,AAR666,99.00" */
    char line[512];
    printf("Please enter the boat data in CSV format                 : ");
    if (!fgets(line, sizeof(line), stdin)) {
        /* EOF or input error */
        return;
    }
    /* Remove trailing newline if any */
    line[strcspn(line, "\n")] = '\0';

    /* We'll parse: name, length, locStr, detail, owed */
    char boatName[128], locStr[32], detailStr[64];
    int length;
    double owed;

    if (5 != sscanf(line, " %[^,],%d,%[^,],%[^,],%lf",
                    boatName, &length, locStr, detailStr, &owed)) {
        printf("Invalid CSV format.\n");
        return;
    }

    if (manager->numBoats >= MAX_BOATS) {
        printf("Cannot add new boat: array is full.\n");
        return;
    }

    Boat *b = createBoat(boatName, length, parseLocationType(locStr), detailStr, owed);
    if (!b) {
        printf("Memory allocation error.\n");
        return;
    }

    /* Add the boat to the end, then re-sort. */
    manager->boats[manager->numBoats] = b;
    manager->numBoats++;
    sortBoatsByName(manager);
}


void removeBoat(BoatManager *manager)
{
    /* Prompt user for boat name */
    char name[128];
    printf("Please enter the boat name                               : ");
    if (!fgets(name, sizeof(name), stdin)) {
        return;
    }
    name[strcspn(name, "\n")] = '\0';

    int idx = findBoatIndex(manager, name);
    if (idx < 0) {
        printf("No boat with that name\n");
        return;
    }

    /* Free the boat's memory */
    free(manager->boats[idx]);
    manager->boats[idx] = NULL;

    /* Shift array elements left to fill the gap */
    for (int i = idx; i < manager->numBoats - 1; i++) {
        manager->boats[i] = manager->boats[i + 1];
    }
    manager->numBoats--;

    /* Re-sort (technically optional after removal, but let's keep it consistent) */
    sortBoatsByName(manager);
}


void acceptPayment(BoatManager *manager)
{
    char name[128];
    printf("Please enter the boat name                               : ");
    if (!fgets(name, sizeof(name), stdin)) {
        return;
    }
    name[strcspn(name, "\n")] = '\0';

    int idx = findBoatIndex(manager, name);
    if (idx < 0) {
        printf("No boat with that name\n");
        return;
    }

    double payment;
    printf("Please enter the amount to be paid                       : ");
    if (scanf("%lf", &payment) != 1) {
        /* flush leftover input */
        while (getchar() != '\n') { }
        return;
    }
    /* flush newline */
    while (getchar() != '\n') { }

    /* Check if payment exceeds amount owed */
    Boat *b = manager->boats[idx];
    if (payment > b->amountOwed) {
        printf("That is more than the amount owed, $%.2f\n", b->amountOwed);
        return;
    }
    /* Subtract the payment */
    b->amountOwed -= payment;
}


void monthlyUpdate(BoatManager *manager)
{
    /* Add monthly charges based on boat length and location */
    for (int i = 0; i < manager->numBoats; i++) {
        Boat *b = manager->boats[i];
        if (!b) continue;

        double charge = 0.0;
        switch (b->locType) {
            case SLIP:
                charge = MONTH_SLIP * b->length;
                break;
            case LAND:
                charge = MONTH_LAND * b->length;
                break;
            case TRAILOR:
                charge = MONTH_TRAILOR * b->length;
                break;
            case STORAGE:
                charge = MONTH_STORAGE * b->length;
                break;
        }
        b->amountOwed += charge;
    }
}


void freeAllBoats(BoatManager *manager)
{
    for (int i = 0; i < manager->numBoats; i++) {
        if (manager->boats[i]) {
            free(manager->boats[i]);
            manager->boats[i] = NULL;
        }
    }
    manager->numBoats = 0;
}


/* --------------------------------------------------------------------------
   main function
   -------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    /* Check for command-line argument: CSV file name. */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <BoatData.csv>\n", argv[0]);
        return 1;
    }

    /* Prepare the BoatManager structure on the stack (no globals!). */
    BoatManager manager;
    manager.numBoats = 0;
    for (int i = 0; i < MAX_BOATS; i++) {
        manager.boats[i] = NULL;
    }

    /* Load data from CSV file, if exists */
    loadFromCSV(&manager, argv[1]);

    /* Print welcome message */
    printf("\n");
    printf("Welcome to the Boat Management System\n");
    printf("-------------------------------------\n\n");

    /* Main menu loop */
    while (1) {
        printf("(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        char cmd[32];
        if (!fgets(cmd, sizeof(cmd), stdin)) {
            /* If EOF, break and save */
            break;
        }
        /* Remove trailing newline */
        cmd[strcspn(cmd, "\n")] = '\0';

        if (cmd[0] == '\0') {
            /* If user just hits enter, do nothing. */
            continue;
        }

        /* Convert first character to lowercase for case-insensitive menu */
        char c = (char)tolower((unsigned char)cmd[0]);
        switch (c) {
            case 'i':
                printInventory(&manager);
                break;
            case 'a':
                addBoat(&manager);
                printf("\n");
                break;
            case 'r':
                removeBoat(&manager);
                printf("\n");
                break;
            case 'p':
                acceptPayment(&manager);
                printf("\n");
                break;
            case 'm':
                monthlyUpdate(&manager);
                printf("\n");
                break;
            case 'x':
                /* Exit the program: save CSV, free memory, then quit. */
                printf("\nExiting the Boat Management System\n");
                printf("\n");
                saveToCSV(&manager, argv[1]);
                freeAllBoats(&manager);
                return 0;
            default:
                /* If invalid menu option, show error. */
                printf("Invalid option %s\n", cmd);
                printf("\n");
                break;
        }
    }

    /* If we reach here, user likely did Ctrl+D or similar. Save and exit. */
    saveToCSV(&manager, argv[1]);
    freeAllBoats(&manager);
    return 0;
}


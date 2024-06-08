//
//  main.c
//  HW3
//
//  Created by Mert Arıcan on 2.12.2023.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define BUFFER 256
#define INSERT_VAL 'i'
#define SEARCH_VAL 's'
#define DELETE_VAL 'd'
#define RELOCATE_VAL 'r'

// MARK: - Data structures, Initializers, Destructors

/*
 * struct: record
 * -------------
 * struct used to represent records
 */
typedef struct record {
    char *name;
    bool deleted;
} Record;

/*
 * function: initRecordWithName
 * -------------------------
 * Function used to initialize and return 'Record'
 * value to the caller, for given 'name'.
 *
 * - Arguments:
 *      - name: String value that the 'record' is going to store.
 *
 * - Returns: 'Record' that stores the given 'name'.
 */
Record initRecordWithName(const char *name) {
    Record record;
    record.name = malloc(sizeof(char) * ((strlen(name)+1)));
    strcpy(record.name, name);
    record.deleted = false;
    return record;
}

/*
 * function: freeRecord
 * --------------------
 * Function used for freeing the memory used by passed 'record' instance.
 *
 * - Arguments:
 *      - record: Pointer to a record
 */
void freeRecord(Record *record) {
    free(record->name);
}

/*
 * struct: HashTable
 * -----------------
 * struct used to represent hash table
 */
typedef struct hash_table {
    Record *values;
    float loadFactor;
    int length;
    int activeRecordCount;
} HashTable;

/*
 * function: createTable
 * --------------------
 * Function that creates a new 'HashTable' instance and returns a pointer to it.
 *
 * - Arguments:
 *      - M: Length of the hash table.
 *      - loadFactor: Load factor determined by user.
 *
 * - Returns: Allocated 'HashTable' instance.
 */
HashTable *createTable(const int M, const float loadFactor) {
    HashTable *table = malloc(sizeof(HashTable));
    Record *records = malloc(sizeof(Record)*M);
    table->values = records;
    table->loadFactor = loadFactor;
    int i = 0;
    for (i = 0; i < M; i++) {
        records[i].name = NULL;
        records[i].deleted = 0;
    }
    table->activeRecordCount = 0;
    table->length = M;
    return table;
}

/*
 * enum: QueryResultStatus
 * -----------------------
 * enum used to represent the status of the result of a query.
 *
 * case 'RECORD_NOT_FOUND': Means record is not in table.
 * case 'RECORD_NOT_FOUND_AND_THE_TABLE_IS_FULL': Means record is not in table, and there is no empty slot in it.
 * case 'PASSIVE_RECORD_FOUND': Means record is in table and is softly deleted.
 * case 'ACTIVE_RECORD_FOUND': Means record is in the table.
 */
typedef enum {
    RECORD_NOT_FOUND,
    RECORD_NOT_FOUND_AND_THE_TABLE_IS_FULL,
    PASSIVE_RECORD_FOUND, // Means 'softly deleted record', i.e., record whose 'deleted' field is set to '1'.
    ACTIVE_RECORD_FOUND
} QueryResultStatus;

/*
 * struct: QueryResult
 * -------------------
 * struct used to represent the result of a query done by using a record with a specific 'name'.
 *
 * - Members:
 *      - slot: 'int' value that stores either:
 *           - if status is 'RECORD_NOT_FOUND': Empty index that is appropriate to use for inserts.
 *           - if status is 'RECORD_NOT_FOUND_AND_THE_TABLE_IS_FULL': Initial hash value created for the record
 *                                                              with a 'name', is occupied by another record.
 *           - if status is 'PASSIVE_RECORD_FOUND': Index that holds the softly deleted record.
 *           - if status is 'ACTIVE_RECORD_FOUND': Index that holds the active record.
 *
 *      - status: member that holds the status of the query result.
 */
typedef struct QueryResult {
    int slot;
    QueryResultStatus status;
} QueryResult;


/*
 * enum: UserAction
 * ----------------
 * enum that enumerates all possible user interactions.
 *
 */
typedef enum {
    UNDEFINED = 0,
    INSERT = INSERT_VAL,
    SEARCH = SEARCH_VAL,
    DELETE = DELETE_VAL,
    RELOCATE = RELOCATE_VAL
} UserAction;



// MARK: - Dealing with Prime Numbers

/*
 * function: isPrime
 * -----------------
 *
 * Checks and returns whether the given 'number' is prime or not.
 *
 * - Arguments
 *      - number: Candidate number
 *
 * - Returns: Whether the given 'number' is prime or not.
 */
bool isPrime(const int number) {
    if (number < 2) { return false; }
    int i = 0;
    for (i=2; i<=number/2; i++) {
        if (number % i == 0) { return false; }
    }
    return true;
}

/*
 * function: firstPrimeThatFollowsGivenNumber
 * ------------------------------------------
 *
 * Returns the first prime number that is >= to the 'number'. This function uses the fact
 * that any prime number (except '2' and '3') can be formulated as '6n-1' or '6n+1'.
 *
 * - Arguments
 *      - number: Lower bound of the prime number.
 *
 * - Returns: The first prime number that is >= to the 'number'
 */
int firstPrimeThatFollowsGivenNumber(const int number) {
    if (number <= 3) {
        return (number <= 2) ? 2 : 3;
    }
    if (number % 6 == 0 && isPrime(number+1)) {
        return number+1;
    }
    int n = (number / 6);
    while (true) {
        if (((6*n)-1 >= number) && isPrime(6*n)-1) {
            return 6*n - 1;
        }
        if (((6*n)+1 >= number) && isPrime((6*n)+1)) {
            return 6*n + 1;
        }
        n++;
    }
    return 0;
}

/*
 * function: getPrimeToBuildTable
 * ------------------------------
 *
 * Function that takes 'N' and 'loadFactor' from user and calculates the table length accordingly.
 *
 * - Arguments
 *      - _loadFactor: pointer to 'loadFactor' value from 'main' function. This value is needed during the program.
 *
 * - Returns: The prime number that is appropriate to use as a length for hash table.
 */
int getPrimeToBuildTable(float *_loadFactor) {
    int N = 0;
    float loadFactor = 0.0;
    bool validN = false;
    bool validLoadFactor = false;
    char input[BUFFER];
    while (!validN) { // Get 'N' from user
        printf("Please enter maximum number of elements that can be added to the table: ");
        scanf(" %d", &N);
        fgets(input, BUFFER, stdin); // Empty the 'stdio'.
        validN = N > 0;
    }
    while (!validLoadFactor) { // Get 'loadFactor' from user
        printf("Please enter a load factor between 0.0 and 1.0: ");
        scanf(" %f", &loadFactor);
        fgets(input, BUFFER, stdin); // Empty the 'stdio'.
        validLoadFactor = loadFactor > 0.0 && loadFactor < 1.0;
    }
    printf("\n"); *_loadFactor = loadFactor;
    return firstPrimeThatFollowsGivenNumber(ceil(((double) N) / loadFactor));
}

// MARK: - Hash functions

/*
 * function: prehash
 * -----------------
 *
 * Function that maps the given set of characters (i.e. name) with a spesific positive integer
 * using Horner's rule. This is the first step of hashing a string. This numeric value later
 * gets used by hash function(s) to produce a valid hash value.
 *
 * - Arguments
 *      - name: string value to map to a number.
 *
 * - Returns: A 'prehash' value of type 'int'. This value is going to get mapped to a valid
 *          key by hash function(s).
 */
int prehash(const char *name, const int M) {
    int i = 0;
    const int PRIME = 31;
    int prehashValue = 0;
    for (i = ((int) strlen(name)-1); i >= 0; i--) {
        prehashValue = (PRIME * prehashValue + name[i]) % M; // Take modulo at each step to prevent overflow.
                                                            // This will not change the overall result.
    }
    return prehashValue;
}

/*
 * function: hash1
 * ---------------
 *
 * Function that first maps a given set of characters (i.e. name) to a numeric value
 * using 'prehash' function, and then maps this number to valid index on hash table
 * by using division method.
 *
 * This function maps the all possible universe of keys (result of 'prehash' function)
 * to valid set of keys (ones that between '0' and 'M'.)
 *
 * > Warning:
 *  Value of this function shouldn't be used directly to store a value, but rather should
 *  be used as a part of 'hash' function.
 *
 * - Arguments:
 *      - name: string value to hash.
 *      - M: length of the hash table that this function creates hash value for.
 *
 * - Returns: Valid hash number to be used in hash function for given 'name'.
 */
int hash1(const char *name, const int M) {
    return prehash(name, M) % M;
}

/*
 * function: hash2
 * ---------------
 *
 * Function that first maps a given set of characters (i.e. name) to a numeric value
 * using 'prehash' function, and then maps this number to valid index on hash table
 * by using division method.
 *
 * This function maps the all possible universe of keys (result of 'prehash' function)
 * to valid set of keys (ones that between '0' and 'M-2'.)
 *
 * > Warning:
 *  Value of this function shouldn't be used directly to store a value, but rather should
 *  be used as a part of 'hash' function.
 *
 * - Arguments:
 *      - name: string value to hash.
 *      - M: length of the hash table that this function creates hash value for.
 *
 * - Returns: Valid hash number to be used in hash function for given 'name'.
 */
int hash2(const char *name, const int M) {
    return 1 + (prehash(name, M) % (M-2));
}

/*
 * function: hash
 * --------------
 *
 * Function that creates hash value for given 'name' considering collisions with other values.
 * This function uses 'Double Hashing' strategy to resolve collisions.
 *
 * For this function to be a valid hash function: (k: arbitrary key, h: hash function, (k, x): pair of key and collision count)
 *  - For all h(k, 1), h(k, 2) all the way up to h(k, M-1), this should be a permutation of 0, 1, ..., M-1.
 *
 * - Arguments:
 *      - name: string value to hash.
 *      - collisionCount: 'int' value that represents the collision count.
 *      - M: length of the hash table that this function creates hash value for.
 *
 * - Returns: Valid hash number for given 'name'.
 */
int hash(const char *name, const int collisionCount, const int M) {
    return (hash1(name, M) + (collisionCount * hash2(name, M))) % M;
}



// MARK: - Hash table functionality

/*
 * function: isThereAnyRecordInSlot
 * --------------------------------
 *
 * Function that returns whether there is any record (whether active or passive) in
 * given 'table' at given 'slot'.
 *
 * - Arguments:
 *      - table: hash table.
 *      - slot: index to look for.
 *
 * - Returns: Whether there is any record (whether active or passive) in
 *              given 'table' at given 'slot'.
 */
bool isThereAnyRecordInSlot(Record *table, const int slot) {
    return (table[slot].name != NULL);
}

/*
 * function: isThereAnyActiveRecordInSlot
 * --------------------------------
 *
 * Function that returns whether there is any active record in
 * given 'table' at given 'slot'.
 *
 * - Arguments:
 *      - table: hash table.
 *      - slot: index to look for.
 *
 * - Returns: Whether there is any active record in
 *              given 'table' at given 'slot'.
 */
bool isThereAnyActiveRecordInSlot(Record *table, const int slot) {
    return ((table[slot].name != NULL) && !(table[slot].deleted));
}

/*
 * function: __search
 * ------------------
 *
 * Provides essential functionality to be used for implementing 'insert', 'search'
 * and 'delete' actions. Returns a value of type 'QueryResult'.
 * 'QueryResult' is a struct that is basically a pair of 'index' and 'status' pairs.
 *
 * - This function returns either:
 *
 *      - if 'name' is in the given 'table', then it is the 'index' of that item
 *          and the 'status' value of 'ACTIVE_RECORD_FOUND'.
 *
 *      - if 'name' is in the given 'table' but it is deleted, then it returns the index
 *           of that item and the 'status' value of 'PASSIVE_RECORD_FOUND'.
 *
 *      - if 'name' is not the given 'table' then it returns the first empty slot index
 *           and the 'status' value of 'RECORD_NOT_FOUND'.
 *
 *      - if 'name' is not in the given 'table', and table is full, then it returns the
 *          initial hash value that is occupied by other value, and the 'status' of
 *          'RECORD_NOT_FOUND_AND_THE_TABLE_IS_FULL'.
 *
 * - Arguments:
 *      - name: Name to search for.
 *      - table: Hash table.
 *      - M: Number of slots in table.
 *
 * > Warning:
 *  Value of this function shouldn't be used directly, but rather should
 *  be used as a part of 'insert', 'search' and 'delete' actions.
 *
 * This function prints all the 'DEBUG' messages (except ones related with 'RELOCATE' action).
 *
 * - Returns: 'QueryResult' value as explained above.
 */
QueryResult __search(const char *name, Record *table, const int M, UserAction action, bool debugMode) {
    int collisionCount = 0;
    const int initialHash = hash(name, collisionCount, M);
    int slot = initialHash;
    if (debugMode) {
        if (!(action == RELOCATE)) {
            printf("\nDEBUG DESCRIPTION FOR %s\n\n", (action == INSERT) ? "INSERT" : (action == SEARCH) ? "SEARCH" : "DELETE");
        }
        printf("h1(\"%s\") = %d\n", name, hash1(name, M));
        printf("h2(\"%s\") = %d\n\n", name, hash2(name, M));
    }
    QueryResult result = { slot, RECORD_NOT_FOUND }; // Initialize result with default values.
    while (isThereAnyRecordInSlot(table, slot)) { // While there is a record in table at position 'slot'...
        Record record = table[slot];
        if (strcmp(name, record.name) == 0) { // If this record has the same as given 'name'...
            result.slot = slot; // Assign the found 'slot' value to 'result'.
            result.status = (record.deleted) ? PASSIVE_RECORD_FOUND : ACTIVE_RECORD_FOUND; // If record is 'deleted' then status is 'passive' else 'active'.
            if (debugMode) {
                if (action == INSERT) {
                    if (result.status == PASSIVE_RECORD_FOUND) {
                        printf("DEBUG: Empty slot for inserting '%s' is found at %d.\n", name, result.slot);
                    }
                    else {
                        printf("DEBUG: '%s' is already in table at position %d.\n", name, result.slot);
                    }
                }
                else if (action == SEARCH) {
                    if (result.status == PASSIVE_RECORD_FOUND) {
                        printf("DEBUG: Couldn't find '%s' in the table.\n", name);
                    }
                    else {
                        printf("DEBUG: '%s' is found at position %d.\n", name, result.slot);
                    }
                }
                else if (action == DELETE) {
                    if (result.status == PASSIVE_RECORD_FOUND) {
                        printf("DEBUG: Couldn't find '%s' in the table to delete.\n", name);
                    }
                    else {
                        printf("DEBUG: '%s' is removed from position %d.\n", name, result.slot);
                    }
                }
            }
            return result;
        }
        else { // means that there is a collision...
            if (debugMode) {
                if (action == INSERT) {
                    printf("DEBUG: Couldn't find empty slot at index: %d to insert '%s'.\n", result.slot, name);
                }
                else if (action == SEARCH) {
                    printf("DEBUG: Couldn't find '%s' at the adress %d.\n", name, result.slot);
                }
                else if (action == DELETE) {
                    printf("DEBUG: Couldn't find '%s' at the adress %d (delete attempt failed).\n", name, result.slot);
                }
            }
            slot = hash(name, ++collisionCount, M); // Compute the new hash value with incremented 'collisionCount'
            result.slot = slot; // update 'slot' value of result
            if (slot == initialHash) {
                // If new has value is same as the initial hash value created for slot, that means there is no empty slot in table.
                result.status = RECORD_NOT_FOUND_AND_THE_TABLE_IS_FULL;
                if (debugMode) {
                    printf("DEBUG: Table is full.\n");
                }
                return result;
            }
        }
    }
    if (debugMode) {
        if (action == INSERT) {
            printf("DEBUG: Empty slot for inserting %s into table is found at index %d.\n", name, result.slot);
        }
        else if (action == SEARCH) {
            printf("DEBUG: Couldn't find '%s' at the adress %d.\n", name, result.slot);
        }
        else if (action == DELETE) {
            printf("DEBUG: Couldn't find '%s' at the adress %d (delete attempt failed).\n", name, result.slot);
        }
    }
    return result;
}

/*
 * function: createAndInsertNewRecordToTable
 * -----------------------------------------
 *
 * Function that creates a new 'Record' value and adds it to the table.
 *
 * - Arguments:
 *      - name: name to insert.
 *      - table: hash table to insert.
 *      - slot: index to insert.
 */
void createAndInsertNewRecordToTable(const char *name, HashTable *table, int slot) {
    Record newRecord = initRecordWithName(name);
    table->values[slot] = newRecord;
}

float currentLoadFactorOfTable(HashTable *table) {
    return (((float) table->activeRecordCount / (float) table->length));
}

// MARK: User Actions

void relocate(HashTable *table, int newLength, bool debugMode); // Prototype needed

/*
 * function: insert
 * ----------------
 *
 * Function that inserts a new 'Record' value with given 'name' to the table.
 *
 * - Arguments:
 *      - name: name to insert.
 *      - table: hash table to insert.
 *      - M: number of slots in hash table.
 */
void insert(const char *name, HashTable *table, bool debugMode) {
    const QueryResult result = __search(name, table->values, table->length, INSERT, debugMode); // Search for given 'name' in hash table
    const int slot = result.slot;
    printf("\n");
    switch (result.status) {
        case RECORD_NOT_FOUND: // Empty slot found to insert given 'name'
            createAndInsertNewRecordToTable(name, table, slot);
            table->activeRecordCount++;
            printf("'%s' inserted into adress: %d.\n", name, slot);
            break;
        case RECORD_NOT_FOUND_AND_THE_TABLE_IS_FULL: // No empty slot in hash table
            printf("Table is full.\n");
            break;
        case PASSIVE_RECORD_FOUND: // Record in table and 'deleted' flag set to 'true'
            table->values[slot].deleted = 0;
            table->activeRecordCount++;
            printf("'%s' inserted into adress: %d.\n", name, slot);
            break;
        case ACTIVE_RECORD_FOUND: // 'name' is already in hash table
            printf("Couldn't insert '%s' into table because it is already in table.\n", name);
            break; // error.
    }
    if (currentLoadFactorOfTable(table) >= table->loadFactor) {
        printf("Current load factor (%f) is bigger than the maximum allowed (%f).\nRelocating records into a bigger table.\n", currentLoadFactorOfTable(table), table->loadFactor);
        printf("New size: %d ||| old size: %d \n", firstPrimeThatFollowsGivenNumber(table->length*2) , table->length);
        relocate(table, firstPrimeThatFollowsGivenNumber(table->length*2), debugMode);
    }
}

/*
 * function: search
 * ----------------
 *
 * Function that searches for a 'Record' value with given 'name' in the table.
 *
 * - Arguments:
 *      - name: name
 *      - table: hash table
 *      - debugMode: whether debug messages should printed or not
 */
void search(const char *name, HashTable *table, bool debugMode) {
    const QueryResult result = __search(name, table->values, table->length, SEARCH, debugMode);
    printf("\n");
    switch (result.status) {
        case RECORD_NOT_FOUND:
        case RECORD_NOT_FOUND_AND_THE_TABLE_IS_FULL:
        case PASSIVE_RECORD_FOUND:
            printf("Couldn't find '%s' in the table.\n", name);
            break;
        case ACTIVE_RECORD_FOUND:
            printf("'%s' is at the adress: %d.\n", name, result.slot);
            break;
    }
}

/*
 * function: delete
 * ----------------
 *
 * Function that deletes a 'Record' with given 'name' from the table.
 *
 * - Arguments:
 *      - name: name
 *      - table: hash table
 *      - debugMode: whether debug messages should printed or not
 */
void delete(const char *name, HashTable *table, bool debugMode) {
    const QueryResult result = __search(name, table->values, table->length, DELETE, debugMode);
    printf("\n");
    switch (result.status) {
        case RECORD_NOT_FOUND:
        case RECORD_NOT_FOUND_AND_THE_TABLE_IS_FULL:
        case PASSIVE_RECORD_FOUND:
            printf("Couldn't find '%s' in the table.\n", name);
            break;
        case ACTIVE_RECORD_FOUND:
            table->values[result.slot].deleted = 1;
            table->activeRecordCount--;
            printf("Removed '%s' from adress: %d.\n", name, result.slot);
            if (currentLoadFactorOfTable(table) <= (table->loadFactor * 0.25)) {
                printf("Current load factor is too low (%f).\nRelocating records into a smaller table.\n", currentLoadFactorOfTable(table));
                relocate(table, firstPrimeThatFollowsGivenNumber(table->length/2), debugMode);
            }
            break;
    }
}

/*
 * function: relocate
 * ------------------
 *
 * Function that relocates the given hash table. Rehashes all the active records
 * into a new table, but not the deleted ones.
 *
 * - Arguments:
 *      - table: pointer to a hash table which is going to get relocated.
 *      - newLength: number of slots in the new hash table
 *      - debugMode: whether debug messages should printed or not
 */
void relocate(HashTable *table, int newLength, bool debugMode) {
    Record *_records = table->values;
    HashTable *newTable = createTable(newLength, table->loadFactor);
    int i = 0;
    printf("\n");
    if (debugMode) {
        printf("DEBUG DESCRIPTION FOR RELOCATE\n");
    }
    for (i = 0; i < table->length; i++) {
        if (isThereAnyRecordInSlot(_records, i)) {
            if (isThereAnyActiveRecordInSlot(_records, i)) {
                QueryResult result = __search(_records[i].name, newTable->values, newLength, RELOCATE, debugMode);
                createAndInsertNewRecordToTable(_records[i].name, newTable, result.slot);
                newTable->activeRecordCount++;
                if (debugMode) {
                    printf("DEBUG: name: '%s' -- deleted: False -- old address: %d -- New address: %d.\n",
                           _records[i].name,
                           i,
                           result.slot);
                }
                printf("Relocating '%s' into new table. (new adress %d)\n",_records[i].name, result.slot);
            }
            else {
                if (debugMode) {
                    printf("DEBUG: name: '%s' -- deleted: True -- old address: %d -- Won't get added to new table.\n",
                           _records[i].name,
                           i);
                }
            }
        }
    }
    Record *tmp = table->values;
    table->values = newTable->values;
    table->activeRecordCount = newTable->activeRecordCount;
    table->length = newTable->length;
    free(tmp);
    free(newTable);
}


// MARK: User Interaction (Text based UI)


/*
 * function: performUserAction
 * ---------------------------
 *
 * Function that performs the desired user action.
 *
 * - Arguments:
 *      - name: name
 *      - table: hash table
 *      - action: action that is going to be performed
 *      - debugMode: whether debug messages should printed or not
 */
void performUserAction(const char *name, HashTable *table, UserAction action, bool debugMode) {
    switch (action) {
        case INSERT:
            insert(name, table, debugMode);
            break;
        case SEARCH:
            search(name, table, debugMode);
            break;
        case DELETE:
            delete(name, table, debugMode);
            break;
        case RELOCATE:
            relocate(table, table->length, debugMode);
            break;
        default:
            break;
    }
}

/*
 * function: interactWithUser
 * --------------------------
 *
 * Function that handles the communication between the program and the user using 'stdio'.
 *
 * - Arguments:
 *      - argc: argc
 *      - argv: argv
 *      - table: hash table
 */
void interactWithUser(int argc, const char *argv[], HashTable *table) {
    // Check whether we are in 'DEBUG' mode or not
    bool debugMode = false;
    if (strcmp(argv[argc-1], "DEBUG") == 0 || strcmp(argv[argc-1], "debug") == 0) {
        debugMode = true;
    }
    
    int i = 0;
    char input[BUFFER];
    char name[BUFFER];
    
    UserAction currentUserAction = UNDEFINED;
    
    while (currentUserAction == UNDEFINED) {
        printf("Press '%c' for creating a new record.\n", INSERT_VAL);
        printf("Press '%c' for deleting a record.\n", DELETE_VAL);
        printf("Press '%c' for searching a record with name.\n", SEARCH_VAL);
        printf("Press '%c' for moving records into a new table.\n", RELOCATE_VAL);
        printf("Press 'e' for exit.\n");
        printf("Action: ");
        
        // All actions require 1 character. If there is more than 1 character then user typed something wrong.
        if (fgets(input, BUFFER, stdin) == NULL || strlen(input) != 2) { // 'strlen' takes '\n' into account also. (this is the reason for checking against 2)
            printf("\nUnexpected command.\n\n");
            continue;
        }
        
        // If input is ...
        switch (input[0]) {
            case INSERT_VAL:
            case DELETE_VAL:
            case SEARCH_VAL:
            case RELOCATE_VAL:
                currentUserAction = input[0]; // any of them then apply the action
                break;
            case 'e': // 'e' is for terminating the program.
                exit(0);
            default: // unknown command
                currentUserAction = UNDEFINED;
                printf("\nUnexpected command.\n\n");
        }
        
        if (currentUserAction == UNDEFINED) { // Ask user again for valid action
            continue;
        }
        
        if (currentUserAction != RELOCATE_VAL) { // If user action is not relocating, then 'name' is also required.
            bool validName = false;
            while (!validName) {
                printf("Please enter a name to %s: ",
                       (currentUserAction == INSERT_VAL) ? "insert" : (currentUserAction == DELETE_VAL) ? "delete" : "search for");
                fgets(name, BUFFER, stdin);
                name[strcspn(name, "\n")] = 0;
                validName = strlen(name) > 0; // careful about strlen !!? ?? ?
                if (!validName) {
                    printf("Invalid name.\n");
                }
            }
        }
        performUserAction(name, table, currentUserAction, debugMode); // perform action
        currentUserAction = UNDEFINED; // reset variable so that the program loop continues
        printf("\n");
    }
    // Free memory used by 'table'
    for (i = 0; i < table->length; i++) {
        freeRecord(&(table->values[i]));
    }
    free(table);
}

int main(int argc, const char * argv[]) {
    float loadFactor = 0.0;
    int M = getPrimeToBuildTable(&loadFactor);
    HashTable *table = createTable(M, loadFactor);
    interactWithUser(argc, argv, table);
    return 0;
}

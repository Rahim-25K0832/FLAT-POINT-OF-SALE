#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROD_FILE "products.dat"
#define SALE_FILE "sales.dat"


typedef struct {
    int pid;
    char pname[50];
    float pquantity;
    char punit[20];
    float pprice;
} Product;

typedef struct {
    int saleid;
    char pname[50];
    float quantitySold;
    char unit[20];
    float pricePerUnit;
    float totalPrice;
    char date[30];
} Sale;


void initFiles() {
    FILE *f1 = fopen(PROD_FILE, "ab");
    if (f1) fclose(f1);

    FILE *f2 = fopen(SALE_FILE, "ab");
    if (f2) fclose(f2);
}

void addProduct() {
    Product p;
    printf("\n[NEW PRODUCT ENTRY]\n");
    printf("Item Code: ");
    scanf("%d", &p.pid);
    getchar(); 

    printf("Name: ");
    fgets(p.pname, sizeof(p.pname), stdin);
    p.pname[strcspn(p.pname, "\n")] = 0; 

    printf("Quantity: ");
    scanf("%f", &p.pquantity);
    getchar();

    printf("Unit (e.g. pcs): ");
    fgets(p.punit, sizeof(p.punit), stdin);
    p.punit[strcspn(p.punit, "\n")] = 0;

    printf("Price: ");
    scanf("%f", &p.pprice);


    FILE *fp = fopen(PROD_FILE, "ab");
    if (fp) {
        fwrite(&p, sizeof(Product), 1, fp);
        fclose(fp);
        printf(">> Product Saved Successfully.\n");
    } else {
        printf(">> Error: Could not access database.\n");
    }
}


void showProducts() {
    Product p;

    FILE *fp = fopen(PROD_FILE, "rb");
    
    if (!fp) {
        printf(">> Database is empty or missing.\n");
        return;
    }

    printf("\n%-5s %-20s %-8s %-8s %-8s\n", "ID", "NAME", "QTY", "UNIT", "PRICE");
    printf("========================================================\n");


    while (fread(&p, sizeof(Product), 1, fp)) {
        printf("%-5d %-20s %-8.2f %-8s %-8.2f\n", 
               p.pid, p.pname, p.pquantity, p.punit, p.pprice);
    }

    printf("========================================================\n");
    fclose(fp);
}

int findProductByID(int target_pid, Product *result) {
    FILE *fp = fopen(PROD_FILE, "rb");
    if (!fp) return 0;

    Product temp;
    int found = 0;
    
    while (fread(&temp, sizeof(Product), 1, fp)) {
        if (temp.pid == target_pid) {
            *result = temp;
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}

int findProductByName(const char *target_name, Product *result) {
    FILE *fp = fopen(PROD_FILE, "rb");
    if (!fp) return 0;

    Product temp;
    int found = 0;

    while (fread(&temp, sizeof(Product), 1, fp)) {
        if (strcmp(temp.pname, target_name) == 0) {
            *result = temp;
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}



void sellProduct() {
    Product current;
    int choice;
    
    printf("\n[TRANSACTION MENU]\n");
    printf("1. Search by ID\n2. Search by Name\nSelect: ");
    scanf("%d", &choice);
    getchar(); 

    int found = 0;
    if (choice == 1) {
        int pid;
        printf("Enter ID: ");
        scanf("%d", &pid);
        found = findProductByID(pid, &current);
    } else if (choice == 2) {
        char name[50];
        printf("Enter Name: ");
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = 0;
        found = findProductByName(name, &current);
    }

    if (!found) {
        printf(">> Item not found in database.\n");
        return;
    }

    printf("Selected: %s | Available Stock: %.2f %s\n", 
           current.pname, current.pquantity, current.punit);

    float qty;
    printf("Quantity to Sell: ");
    scanf("%f", &qty);

    if (qty <= 0 || qty > current.pquantity) {
        printf(">> Error: Invalid Quantity.\n");
        return;
    }

    // 1. UPDATE STOCK (Read Original -> Write Temp)
    FILE *fp = fopen(PROD_FILE, "rb");
    FILE *temp = fopen("temp_db.dat", "wb");
    Product tempProd;

    while (fread(&tempProd, sizeof(Product), 1, fp)) {
        if (tempProd.pid == current.pid) {
            tempProd.pquantity -= qty;
        }
        fwrite(&tempProd, sizeof(Product), 1, temp); 
    }

    fclose(fp);
    fclose(temp);
    remove(PROD_FILE);
    rename("temp_db.dat", PROD_FILE);

    FILE *sp = fopen(SALE_FILE, "ab");
    if (sp) {
        Sale s;
        s.saleid = (int)time(NULL); 
        strcpy(s.pname, current.pname);
        s.quantitySold = qty;
        strcpy(s.unit, current.punit);
        s.pricePerUnit = current.pprice;
        s.totalPrice = qty * current.pprice;

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        sprintf(s.date, "%04d-%02d-%02d %02d:%02d", 
            tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min);

        fwrite(&s, sizeof(Sale), 1, sp);
        fclose(sp);
        printf(">> Sale Finalized. Total Amount: %.2f\n", s.totalPrice);
    }
}


void modifyOrDeleteProduct() {
    int pid_target;
    printf("\n[EDIT DATABASE]\n");
    printf("Enter Product ID: ");
    scanf("%d", &pid_target);

    FILE *fp = fopen(PROD_FILE, "rb");
    FILE *temp = fopen("temp_db.dat", "wb");
    Product p;
    int found = 0;

    if (!fp) {
         printf(">> Error: Database unavailable.\n"); 
         return; 
    }

    while (fread(&p, sizeof(Product), 1, fp)) {
        if (p.pid == pid_target) {
            found = 1;
            printf("Found Item: %s\n", p.pname);
            printf("Action: 1. Modify Details  2. Delete Item\nInput: ");
            int action;
            scanf("%d", &action);

            if (action == 1) {
                // Modify: Update the struct, then write it
                printf("New Price: ");
                scanf("%f", &p.pprice);
                printf("New Quantity: ");
                scanf("%f", &p.pquantity);
                fwrite(&p, sizeof(Product), 1, temp); 
                printf(">> Record Updated.\n");
            } 
            else if (action == 2) {

                printf(">> Record Deleted from Database.\n");
            }
        } else {

            fwrite(&p, sizeof(Product), 1, temp); 
        }
    }

    fclose(fp);
    fclose(temp);

    remove(PROD_FILE);
    rename("temp_db.dat", PROD_FILE);

    if (!found) printf(">> ID Not Found.\n");
}

void mainMenu() {
    int choice;
    initFiles(); 
    while(1) {
        printf("\n=== STORE INVENTORY KERNEL ===\n"); // Unique Title
        printf("1. Add new item\n");
        printf("2. Show all product\n");
        printf("3. Process New Sale\n");
        printf("4. Edit or Remove Item\n");
        printf("5. Exit\n");
        printf("Command: ");
        
        if (scanf("%d", &choice) != 1) break;
        getchar(); // consume buffer

        switch(choice) {
            case 1: addProduct(); break;
            case 2: showProducts(); break;
            case 3: sellProduct(); break;
            case 4: modifyOrDeleteProduct(); break;
            case 5: 
                printf("Closing System...\n");
                exit(0);
            default: 
                printf("Unknown Command.\n");
        }
    }
}

int main() {
    mainMenu();
    return 0;
}


e(PROD_FILE);
    rename("temp_db.dat", PROD_FILE);

    if (!found) printf(">> ID Not Found.\n");
}

void mainMenu() {
    int choice;
    initFiles(); 
    while(1) {
        printf("\n=== STORE INVENTORY KERNEL ===\n"); // Unique Title
        printf("1. Add new item\n");
        printf("2. Show all product\n");
        printf("3. Process New Sale\n");
        printf("4. Edit or Remove Item\n");
        printf("5. Exit\n");
        printf("Command: ");
        
        if (scanf("%d", &choice) != 1) break;
        getchar(); // consume buffer

        switch(choice) {
            case 1: addProduct(); break;
            case 2: showProducts(); break;
            case 3: sellProduct(); break;
            case 4: modifyOrDeleteProduct(); break;
            case 5: 
                printf("Closing Syste
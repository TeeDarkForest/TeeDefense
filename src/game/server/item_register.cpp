#include "gamecontext.h"
#include "entities/turret.h"

void ResetResource(int *Resource)
{
    for (int i = 0; i < NUM_ITEM; i++)
    {
        Resource[i] = 0;
    }
}

void CGameContext::InitItems()
{
    int Resource[NUM_ITEM];
    ResetResource(Resource);
    // Dont move this item!!!
    CreateItem("HAND", // Name
               0,            // ID
               ITYPE_MATERIAL,  // ItemType
               0,            // Damage
               0,    // Level
               -1,           // TurretType
               100,           // Proba
               -1,           // Speed
               Resource);
    // Register Items.
    ResetResource(Resource);
    CreateItem("Log",          // Name
               ITEM_LOG,       // ID
               ITYPE_MATERIAL, // ItemType
               0,              // Damage
               LEVEL_LOG,      // Level
               -1,             // TurretType
               100,            // Proba
               -1,             // Speed
               Resource);
    CreateItem("Coal",         // Name
               ITEM_COAL,      // ID
               ITYPE_MATERIAL, // ItemType
               0,              // Damage
               LEVEL_LOG,      // Level
               -1,             // TurretType
               100,            // Proba
               -1,             // Speed
               Resource);
    CreateItem("Copper",       // Name
               ITEM_COPPER,    // ID
               ITYPE_MATERIAL, // ItemType
               0,              // Damage
               LEVEL_LOG,      // Level
               -1,             // TurretType
               100,            // Proba
               -1,             // Speed
               Resource);
    CreateItem("Iron",         // Name
               ITEM_IRON,      // ID
               ITYPE_MATERIAL, // ItemType
               0,              // Damage
               LEVEL_LOG,      // Level
               -1,             // TurretType
               100,            // Proba
               -1,             // Speed
               Resource);
    CreateItem("Golden",       // Name
               ITEM_GOLDEN,    // ID
               ITYPE_MATERIAL, // ItemType
               0,              // Damage
               LEVEL_LOG,      // Level
               -1,             // TurretType
               100,            // Proba
               -1,             // Speed
               Resource);
    CreateItem("Diamond",      // Name
               ITEM_DIAMOND,   // ID
               ITYPE_MATERIAL, // ItemType
               0,              // Damage
               LEVEL_LOG,      // Level
               -1,             // TurretType
               100,            // Proba
               -1,             // Speed
               Resource);
    CreateItem("Enegry",       // Name
               ITEM_ENEGRY,    // ID
               ITYPE_MATERIAL, // ItemType
               0,              // Damage
               LEVEL_LOG,      // Level
               -1,             // TurretType
               100,            // Proba
               -1,             // Speed
               Resource);
    CreateItem("Zombie Heart",   // Name
               ITEM_ZOMBIEHEART, // ID
               ITYPE_MATERIAL,   // ItemType
               0,                // Damage
               LEVEL_LOG,        // Level
               -1,               // TurretType
               100,              // Proba
               -1,               // Speed
               Resource);
    Resource[ITEM_LOG] = 25;
    CreateItem("wooden sword", // Name
               ITEM_SWORD_LOG, // ID
               ITYPE_SWORD,    // ItemType
               2,              // Damage
               LEVEL_LOG,      // Level
               -1,             // TurretType
               90,             // Proba
               -1,             // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 10;
    CreateItem("wooden axe", // Name
               ITEM_AXE_LOG, // ID
               ITYPE_AXE,    // ItemType
               0,            // Damage
               LEVEL_LOG,    // Level
               -1,           // TurretType
               90,           // Proba
               10,           // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 20;
    CreateItem("wooden pickaxe", // Name
               ITEM_PICKAXE_LOG, // ID
               ITYPE_PICKAXE,    // ItemType
               0,                // Damage
               LEVEL_LOG,        // Level
               -1,               // TurretType
               90,               // Proba
               200,              // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 60;
    Resource[ITEM_COPPER] = 25;
    CreateItem("copper axe",    // Name
               ITEM_AXE_COPPER, // ID
               ITYPE_AXE,       // ItemType
               0,               // Damage
               LEVEL_COPPER,    // Level
               -1,              // TurretType
               90,              // Proba
               17,              // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 70;
    Resource[ITEM_COPPER] = 25;
    CreateItem("copper pickaxe",    // Name
               ITEM_PICKAXE_COPPER, // ID
               ITYPE_PICKAXE,       // ItemType
               0,                   // Damage
               LEVEL_COPPER,        // Level
               -1,                  // TurretType
               90,                  // Proba
               4000,                 // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 1900;
    Resource[ITEM_IRON] = 25;
    CreateItem("iron sword",    // Name
               ITEM_SWORD_IRON, // ID
               ITYPE_SWORD,     // ItemType
               7,               // Damage
               LEVEL_IRON,      // Level
               -1,              // TurretType
               90,              // Proba
               0,               // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 1780;
    Resource[ITEM_IRON] = 25;
    CreateItem("iron axe",    // Name
               ITEM_AXE_IRON, // ID
               ITYPE_AXE,     // ItemType
               0,             // Damage
               LEVEL_IRON,    // Level
               -1,            // TurretType
               70,            // Proba
               24,            // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 1800;
    Resource[ITEM_IRON] = 25;
    CreateItem("iron pickaxe",    // Name
               ITEM_PICKAXE_IRON, // ID
               ITYPE_PICKAXE,     // ItemType
               0,                 // Damage
               LEVEL_IRON,        // Level
               -1,                // TurretType
               90,                // Proba
               5000,               // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 2440;
    Resource[ITEM_GOLDEN] = 25;
    CreateItem("golden sword",    // Name
               ITEM_SWORD_GOLDEN, // ID
               ITYPE_SWORD,       // ItemType
               10,                // Damage
               LEVEL_GOLD,        // Level
               -1,                // TurretType
               80,                // Proba
               1500,              // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 2000;
    Resource[ITEM_GOLDEN] = 10;
    CreateItem("golden axe",    // Name
               ITEM_AXE_GOLDEN, // ID
               ITYPE_AXE,       // ItemType
               0,               // Damage
               LEVEL_GOLD,      // Level
               -1,              // TurretType
               80,              // Proba
               30,              // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 2300;
    Resource[ITEM_GOLDEN] = 25;
    CreateItem("golden pickaxe",    // Name
               ITEM_PICKAXE_GOLDEN, // ID
               ITYPE_PICKAXE,       // ItemType
               0,                   // Damage
               LEVEL_GOLD,          // Level
               -1,                  // TurretType
               90,                  // Proba
               7500,                // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 4400;
    Resource[ITEM_DIAMOND] = 25;
    CreateItem("diamond sword",    // Name
               ITEM_SWORD_DIAMOND, // ID
               ITYPE_SWORD,        // ItemType
               20,                 // Damage
               LEVEL_DIAMOND,      // Level
               -1,                 // TurretType
               90,                 // Proba
               0,                  // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 4000;
    Resource[ITEM_DIAMOND] = 25;
    CreateItem("diamond axe",    // Name
               ITEM_AXE_DIAMOND, // ID
               ITYPE_AXE,        // ItemType
               0,                // Damage
               LEVEL_DIAMOND,    // Level
               -1,               // TurretType
               90,               // Proba
               50,               // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 4200;
    Resource[ITEM_DIAMOND] = 25;
    CreateItem("diamond pickaxe",    // Name
               ITEM_PICKAXE_DIAMOND, // ID
               ITYPE_PICKAXE,        // ItemType
               0,                    // Damage
               LEVEL_DIAMOND,        // Level
               -1,                   // TurretType
               80,                   // Proba
               15000,                 // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 5000;
    Resource[ITEM_DIAMOND] = 50;
    Resource[ITEM_ENEGRY] = 100;
    CreateItem("enegry sword",    // Name
               ITEM_SWORD_ENEGRY, // ID
               ITYPE_SWORD,       // ItemType
               38,                // Damage
               LEVEL_ENEGRY,      // Level
               -1,                // TurretType
               90,                // Proba
               0,                 // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 4666;
    Resource[ITEM_DIAMOND] = 35;
    Resource[ITEM_ENEGRY] = 10;
    CreateItem("enegry pickaxe",    // Name
               ITEM_PICKAXE_ENEGRY, // ID
               ITYPE_PICKAXE,       // ItemType
               0,                   // Damage
               LEVEL_ENEGRY,        // Level
               -1,                  // TurretType
               100,                 // Proba
               250000,               // Speed
               Resource);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 50;
    Resource[ITEM_COPPER] = 10;
    CreateItem("gun turret",    // Name
               ITEM_GUN_TURRET, // ID
               ITYPE_TURRET,    // ItemType
               0,               // Damage
               LEVEL_LOG,       // Level
               TURRET_GUN,        // TurretType
               90,              // Proba
               0,               // Speed
               Resource, 400);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 75;
    Resource[ITEM_COPPER] = 20;
    CreateItem("shotgun turret",    // Name
               ITEM_SHOTGUN_TURRET, // ID
               ITYPE_TURRET,        // ItemType
               0,                   // Damage
               LEVEL_LOG,           // Level
               TURRET_SHOTGUN,      // TurretType
               90,                  // Proba
               0,                   // Speed
               Resource, 400);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 1;
    Resource[ITEM_COPPER] = 600;
    Resource[ITEM_GOLDEN] = 40;
    Resource[ITEM_DIAMOND] = 20;
    Resource[ITEM_ENEGRY] = 5;
    CreateItem("laser turret",    // Name
               ITEM_LASER_TURRET, // ID
               ITYPE_TURRET,      // ItemType
               0,                 // Damage
               LEVEL_DIAMOND,     // Level
               TURRET_LASER,      // TurretType
               90,                // Proba
               0,                 // Speed
               Resource, 220);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 20;
    Resource[ITEM_COPPER] = 500;
    Resource[ITEM_DIAMOND] = 500;
    Resource[ITEM_ENEGRY] = 35;
    CreateItem("laser2077 turret",     // Name
               ITEM_LASER_TURRET_2077, // ID
               ITYPE_TURRET,           // ItemType
               0,                      // Damage
               LEVEL_ENEGRY,           // Level
               TURRET_LASER_2077,      // TurretType
               90,                     // Proba
               0,                      // Speed
               Resource, 200);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 50;
    Resource[ITEM_COAL] = 500;
    Resource[ITEM_GOLDEN] = 250;
    Resource[ITEM_DIAMOND] = 75;
    Resource[ITEM_ENEGRY] = 5;
    CreateItem("follow grenade turret",    // Name
               ITEM_FOLLOW_GRENADE_TURRET, // ID
               ITYPE_TURRET,               // ItemType
               0,                          // Damage
               LEVEL_IRON,                 // Level
               TURRET_FOLLOW_GRENADE,      // TurretType
               90,                         // Proba
               0,                          // Speed
               Resource, 900);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 700;
    Resource[ITEM_COAL] = 1000;
    Resource[ITEM_COPPER] = 70;
    Resource[ITEM_ENEGRY] = 10;
    CreateItem("freeze gun turret", // Name
               ITEM_FGUN_TURRET,    // ID
               ITYPE_TURRET,        // ItemType
               0,                   // Damage
               LEVEL_ENEGRY,        // Level
               TURRET_LASER_2077,   // TurretType
               90,                  // Proba
               0,                   // Speed
               Resource, 400);
    ResetResource(Resource);
    Resource[ITEM_LOG] = 700;
    Resource[ITEM_COAL] = 300;
    Resource[ITEM_COPPER] = 900;
    Resource[ITEM_IRON] = 700;
    Resource[ITEM_DIAMOND] = 30;
    Resource[ITEM_ENEGRY] = 35;
    CreateItem("shotgun2077 turret",     // Name
               ITEM_SHOTGUN_TURRET_2077, // ID
               ITYPE_TURRET,             // ItemType
               0,                        // Damage
               LEVEL_GOLD,               // Level
               TURRET_SHOTGUN_2077,      // TurretType
               90,                       // Proba
               0,                        // Speed
               Resource, 400);
}
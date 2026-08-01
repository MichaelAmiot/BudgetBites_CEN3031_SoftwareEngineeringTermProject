# BudgetBites data layout

The application reads the versioned CSV catalog in `seed/` directly. No
database build step or external data service is required after cloning.

```
data/
├── seed/                       # Shared, read-only recipe catalog
│   ├── recipes.csv
│   ├── ingredients.csv
│   ├── recipe_ingredients.csv
│   ├── recipe_instructions.csv
│   ├── seasoner.csv
│   ├── recipes_seasoner.csv
│   ├── seasoner_allergens.csv
│   ├── seasoner_dietary_tags.csv
│   ├── allergens.csv
│   ├── ingredient_allergens.csv
│   ├── dietary_tags.csv
│   ├── ingredient_dietary_tags.csv
│   └── Sources/                # Raw downloaded or parsed source material
└── local/                      # Per-user CSV data; ignored by Git
    ├── users.csv               # Credentials and profile image paths
    ├── user_info.csv
    ├── user_dietary_tags.csv
    ├── user_allergens.csv
    └── user_pantry_items.csv
```

`RecipeDataBase` loads the catalog CSV files once at startup and builds
in-memory indexes for recipe, ingredient, seasoner, dietary, and allergen
queries. Compatibility filtering checks confirmed conflicts from both
ingredients and seasoners.
`UserInfoRepository` reads and writes the local user CSV files separately, so
catalog updates never overwrite a user's budget, preferences, or pantry.
`users.csv` is owned by `UserRepository`; the four supplemental tables use the
username as their link to that account.

The raw source files in `seed/Sources/` are retained for catalog maintenance.
They are not read by the application at runtime.

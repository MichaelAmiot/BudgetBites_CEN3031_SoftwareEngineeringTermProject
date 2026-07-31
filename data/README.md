# BudgetBites data layout

`budgetbites.db` is the versioned SQLite catalog distributed with the project. After cloning the repository, the
application should open `data/budgetbites.db`
directly; no seed-import step is required for normal use.

```
data/
├── budgetbites.db              # Runtime SQLite catalog, committed to Git
├── migrations/                 # SQLite schema used to rebuild the catalog
│   └── 001_menu_catalog.sql
├── scripts/
│   └── build_seed_database.py  # Maintainer-only catalog rebuild command
├── seed/                       # Editable, reviewable database seed data
│   ├── recipes.csv
│   ├── ingredients.csv
│   ├── recipe_ingredients.csv
│   ├── recipe_instructions.csv
│   ├── seasoner.csv
│   ├── recipes_seasoner.csv
│   ├── allergens.csv
│   ├── ingredient_allergens.csv
│   ├── dietary_tags.csv
│   ├── ingredient_dietary_tags.csv
│   └── Sources/                # Raw downloaded or parsed source material
└── local/                      # Developer-only databases; ignored by Git
```

## Maintaining

`seed/` is the source of truth. When a maintainer changes seed data or the schema, rebuild the checked-in catalog from
the project root:

```bash
python3 data/scripts/build_seed_database.py
```

The builder creates `data/budgetbites.db` atomically and validates imported row counts, foreign keys, and SQLite
integrity before replacing the prior file. Commit the changed seed files, migration or builder changes when applicable,
and the rebuilt `data/budgetbites.db` together.

The raw source downloads and parsed source material are stored in
`seed/Sources/`. They support future catalog maintenance, but the runtime application and the database builder do not
read them directly.

## Database contents

The database includes recipe metadata, normalized ingredients with prices and standard purchase units, recipe ingredient
quantities and weights, preparation instructions, seasoners, allergen mappings, dietary-tag mappings, and views that
expose recipe ingredient names or dietary/allergen conflicts.

# BudgetBites data layout

Keep database structure and editable seed data under version control. Keep
runtime databases and any personal data out of version control.

```
data/
├── migrations/                 # Ordered SQLite schema migrations, e.g. 001_menu_catalog.sql
├── scripts/                    # Repeatable seed import helpers
├── seed/                       # Human-reviewable catalog data used to build a database
│   ├── recipes.csv             # Sprint 1 recipe metadata and source attribution
│   ├── recipes_parsed.json     # Raw Wikibooks parse; import input, not app data
│   ├── ingredients.csv         # Normalized ingredient catalog
│   ├── recipe_ingredients.csv  # Recipe-to-ingredient quantities and units
│   ├── allergens.csv           # Controlled allergy vocabulary
│   ├── ingredient_allergens.csv # Ingredient-to-allergen assessments
│   ├── dietary_tags.csv        # Controlled dietary/exclusion vocabulary
│   ├── ingredient_dietary_tags.csv # Ingredient-to-dietary-tag assessments
│   └── recipe_steps.csv        # Added after recipe-page review
├── attribution/                # Source and license records for imported content
└── local/                      # Developer/runtime databases; ignored by Git
    └── budgetbites.db
```

`seed/` is the source of truth for catalog content. Build the ignored local
database with `python3 data/scripts/build_seed_database.py`; it applies the
migration and imports all seed CSV files in a transaction. Do not edit the
SQLite file by hand.

The current `seed/recipes.csv` contains candidate metadata selected from the
Wikibooks Cookbook. Its estimated servings and durations are intentionally
marked `needs_recipe_page_review`; retain the provided source URL and the
CC BY-SA license attribution when each recipe is completed.

`ingredients.csv` and `recipe_ingredients.csv` cover all 90 current recipes.
They were derived from `recipes_parsed.json`; six pages whose ingredient
sections were absent from that parser snapshot were transcribed from their
linked Wikibooks recipe pages. Quantities left blank mean the source listed an
ingredient without a measurable amount (for example, “salt to taste”).

Allergy and dietary records are deliberately conservative. In the two
ingredient association tables, a missing row means `unknown`, never “free
from”. The initial seed does not infer `may_contain` from recipe text.
Product-specific or composite items are marked `needs_review` where an
ingredient name alone is not enough to make a reliable claim.

`ingredient_allergens.status` is one of `contains`, `derived_from`,
`may_contain`, or `unknown`. `ingredient_dietary_tags.status` is one of
`compatible`, `incompatible`, `conditional`, or `unknown`. In both association
tables, `review_status` is `verified` or `needs_review`.

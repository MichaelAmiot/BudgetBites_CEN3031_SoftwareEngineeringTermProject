PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS schema_migrations (
    version INTEGER PRIMARY KEY,
    filename TEXT NOT NULL UNIQUE,
    applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS recipes (
    id INTEGER PRIMARY KEY,
    title TEXT NOT NULL UNIQUE,
    servings INTEGER,
    prep_minutes INTEGER,
    cook_minutes INTEGER,
    difficulty TEXT NOT NULL,
    meal_type TEXT NOT NULL,
    primary_equipment TEXT NOT NULL,
    selection_notes TEXT NOT NULL,
    source_name TEXT NOT NULL,
    source_url TEXT NOT NULL,
    source_license TEXT NOT NULL,
    review_status TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS ingredients (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    description TEXT NOT NULL,
    price_100gm REAL NOT NULL CHECK (price_100gm >= 0),
    purchase_unit_gram INTEGER NOT NULL CHECK (purchase_unit_gram > 0),
    purchase_unit_label TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS recipe_ingredients (
    id INTEGER PRIMARY KEY,
    recipe_id INTEGER NOT NULL,
    ingredient_id INTEGER NOT NULL,
    quantity REAL NOT NULL CHECK (quantity >= 0),
    unit TEXT NOT NULL,
    weight_gram REAL CHECK (weight_gram IS NULL OR weight_gram >= 0),
    source_ingredient_text TEXT NOT NULL,
    match_type TEXT NOT NULL,
    FOREIGN KEY (recipe_id) REFERENCES recipes(id) ON DELETE CASCADE,
    FOREIGN KEY (ingredient_id) REFERENCES ingredients(id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS recipe_instructions (
    recipe_id INTEGER PRIMARY KEY,
    preparation_instructions TEXT NOT NULL,
    FOREIGN KEY (recipe_id) REFERENCES recipes(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS seasoner (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    source_examples TEXT NOT NULL,
    review_status TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS recipes_seasoner (
    recipe_id INTEGER PRIMARY KEY,
    seasoners TEXT NOT NULL,
    FOREIGN KEY (recipe_id) REFERENCES recipes(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS allergens (
    id INTEGER PRIMARY KEY,
    code TEXT NOT NULL UNIQUE,
    display_name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS ingredient_allergens (
    ingredient_id INTEGER NOT NULL,
    allergen_id INTEGER NOT NULL,
    status TEXT NOT NULL CHECK (status IN ('contains', 'derived_from', 'may_contain', 'unknown')),
    note TEXT NOT NULL,
    PRIMARY KEY (ingredient_id, allergen_id),
    FOREIGN KEY (ingredient_id) REFERENCES ingredients(id) ON DELETE CASCADE,
    FOREIGN KEY (allergen_id) REFERENCES allergens(id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS dietary_tags (
    id INTEGER PRIMARY KEY,
    code TEXT NOT NULL UNIQUE,
    display_name TEXT NOT NULL,
    tag_group TEXT NOT NULL,
    description TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS ingredient_dietary_tags (
    ingredient_id INTEGER NOT NULL,
    dietary_tag_id INTEGER NOT NULL,
    status TEXT NOT NULL CHECK (status IN ('compatible', 'incompatible', 'conditional', 'unknown')),
    note TEXT NOT NULL,
    PRIMARY KEY (ingredient_id, dietary_tag_id),
    FOREIGN KEY (ingredient_id) REFERENCES ingredients(id) ON DELETE CASCADE,
    FOREIGN KEY (dietary_tag_id) REFERENCES dietary_tags(id) ON DELETE RESTRICT
);

CREATE INDEX IF NOT EXISTS idx_recipe_ingredients_recipe_id
    ON recipe_ingredients (recipe_id);
CREATE INDEX IF NOT EXISTS idx_recipe_ingredients_ingredient_id
    ON recipe_ingredients (ingredient_id);
CREATE INDEX IF NOT EXISTS idx_ingredient_allergens_allergen_id
    ON ingredient_allergens (allergen_id);
CREATE INDEX IF NOT EXISTS idx_ingredient_dietary_tags_tag_id
    ON ingredient_dietary_tags (dietary_tag_id);

CREATE VIEW IF NOT EXISTS recipe_ingredient_details AS
SELECT
    recipe_ingredients.id,
    recipe_ingredients.recipe_id,
    recipes.title AS recipe_name,
    recipe_ingredients.ingredient_id,
    ingredients.name AS ingredient_name,
    recipe_ingredients.quantity,
    recipe_ingredients.unit,
    recipe_ingredients.weight_gram,
    recipe_ingredients.source_ingredient_text,
    recipe_ingredients.match_type
FROM recipe_ingredients
JOIN recipes ON recipes.id = recipe_ingredients.recipe_id
JOIN ingredients ON ingredients.id = recipe_ingredients.ingredient_id;

CREATE VIEW IF NOT EXISTS recipe_allergen_ingredients AS
SELECT
    recipe_ingredients.recipe_id,
    ingredient_allergens.allergen_id,
    recipe_ingredients.ingredient_id,
    ingredient_allergens.status
FROM recipe_ingredients
JOIN ingredient_allergens
    ON ingredient_allergens.ingredient_id = recipe_ingredients.ingredient_id;

CREATE VIEW IF NOT EXISTS recipe_dietary_conflicts AS
SELECT
    recipe_ingredients.recipe_id,
    ingredient_dietary_tags.dietary_tag_id,
    recipe_ingredients.ingredient_id,
    ingredient_dietary_tags.status
FROM recipe_ingredients
JOIN ingredient_dietary_tags
    ON ingredient_dietary_tags.ingredient_id = recipe_ingredients.ingredient_id
WHERE ingredient_dietary_tags.status IN ('incompatible', 'conditional');

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
    default_unit TEXT NOT NULL,
    category TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS recipe_ingredients (
    recipe_id INTEGER NOT NULL,
    ingredient_id INTEGER NOT NULL,
    quantity REAL,
    unit TEXT,
    PRIMARY KEY (recipe_id, ingredient_id),
    FOREIGN KEY (recipe_id) REFERENCES recipes(id) ON DELETE CASCADE,
    FOREIGN KEY (ingredient_id) REFERENCES ingredients(id) ON DELETE RESTRICT,
    CHECK (quantity IS NULL OR quantity >= 0)
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
    review_status TEXT NOT NULL CHECK (review_status IN ('verified', 'needs_review')),
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
    review_status TEXT NOT NULL CHECK (review_status IN ('verified', 'needs_review')),
    note TEXT NOT NULL,
    PRIMARY KEY (ingredient_id, dietary_tag_id),
    FOREIGN KEY (ingredient_id) REFERENCES ingredients(id) ON DELETE CASCADE,
    FOREIGN KEY (dietary_tag_id) REFERENCES dietary_tags(id) ON DELETE RESTRICT
);

CREATE INDEX IF NOT EXISTS idx_recipe_ingredients_ingredient_id
    ON recipe_ingredients (ingredient_id);
CREATE INDEX IF NOT EXISTS idx_ingredient_allergens_allergen_id
    ON ingredient_allergens (allergen_id);
CREATE INDEX IF NOT EXISTS idx_ingredient_dietary_tags_tag_id
    ON ingredient_dietary_tags (dietary_tag_id);

CREATE VIEW IF NOT EXISTS recipe_allergen_ingredients AS
SELECT
    recipe_ingredients.recipe_id,
    ingredient_allergens.allergen_id,
    recipe_ingredients.ingredient_id,
    ingredient_allergens.status,
    ingredient_allergens.review_status
FROM recipe_ingredients
JOIN ingredient_allergens
    ON ingredient_allergens.ingredient_id = recipe_ingredients.ingredient_id;

CREATE VIEW IF NOT EXISTS recipe_dietary_conflicts AS
SELECT
    recipe_ingredients.recipe_id,
    ingredient_dietary_tags.dietary_tag_id,
    recipe_ingredients.ingredient_id,
    ingredient_dietary_tags.status,
    ingredient_dietary_tags.review_status
FROM recipe_ingredients
JOIN ingredient_dietary_tags
    ON ingredient_dietary_tags.ingredient_id = recipe_ingredients.ingredient_id
WHERE ingredient_dietary_tags.status IN ('incompatible', 'conditional');

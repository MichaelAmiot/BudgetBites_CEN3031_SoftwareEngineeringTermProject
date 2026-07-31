#!/usr/bin/env python3
"""Build the versioned BudgetBites SQLite catalog from the editable seed files."""

from __future__ import annotations

import csv
import json
import os
import sqlite3
from pathlib import Path
from typing import Callable


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DATA_DIR = PROJECT_ROOT / "data"
SEED_DIR = DATA_DIR / "seed"
MIGRATION_FILE = DATA_DIR / "migrations" / "001_menu_catalog.sql"
DATABASE_FILE = DATA_DIR / "budgetbites.db"
TEMP_DATABASE_FILE = DATA_DIR / "budgetbites.tmp.db"


def read_csv(filename: str, expected_headers: list[str]) -> list[dict[str, str]]:
    with (SEED_DIR / filename).open(newline="", encoding="utf-8-sig") as source:
        reader = csv.DictReader(source)
        if reader.fieldnames != expected_headers:
            raise ValueError(f"{filename} headers do not match expected schema: {reader.fieldnames}")
        return list(reader)


def integer(value: str) -> int | None:
    return int(value) if value else None


def real(value: str) -> float | None:
    return float(value) if value else None


def import_rows(
    connection: sqlite3.Connection,
    filename: str,
    headers: list[str],
    table: str,
    columns: list[str] | None = None,
    transforms: dict[str, Callable[[str], object]] | None = None,
) -> int:
    rows = read_csv(filename, headers)
    columns = columns or headers
    transforms = transforms or {}
    values = [
        tuple(transforms.get(column, lambda value: value)(row[column]) for column in columns)
        for row in rows
    ]
    placeholders = ", ".join("?" for _ in columns)
    connection.executemany(
        f"INSERT INTO {table} ({', '.join(columns)}) VALUES ({placeholders})",
        values,
    )
    return len(values)


def scalar(connection: sqlite3.Connection, query: str) -> int:
    return int(connection.execute(query).fetchone()[0])


def build_database() -> dict[str, int]:
    if TEMP_DATABASE_FILE.exists():
        TEMP_DATABASE_FILE.unlink()

    connection = sqlite3.connect(TEMP_DATABASE_FILE)
    try:
        connection.execute("PRAGMA foreign_keys = ON")
        connection.executescript(MIGRATION_FILE.read_text(encoding="utf-8"))
        connection.execute("BEGIN")
        connection.execute(
            "INSERT INTO schema_migrations (version, filename) VALUES (?, ?)",
            (1, MIGRATION_FILE.name),
        )
        counts = {
            "recipes": import_rows(
                connection, "recipes.csv",
                ["id", "title", "servings", "prep_minutes", "cook_minutes", "difficulty", "meal_type",
                 "primary_equipment", "selection_notes", "source_name", "source_url", "source_license", "review_status"],
                "recipes", transforms={"id": int, "servings": integer, "prep_minutes": integer, "cook_minutes": integer},
            ),
            "ingredients": import_rows(
                connection, "ingredients.csv",
                ["id", "name", "description", "price_100gm", "purchase_unit_gram", "purchase_unit_label"],
                "ingredients", transforms={"id": int, "price_100gm": real, "purchase_unit_gram": int},
            ),
            "recipe_ingredients": import_rows(
                connection, "recipe_ingredients.csv",
                ["recipe_id", "recipe_name", "ingredient_id", "ingredient_name", "quantity", "unit", "weight_gram", "source_ingredient_text", "match_type"],
                "recipe_ingredients",
                columns=["recipe_id", "ingredient_id", "quantity", "unit", "weight_gram", "source_ingredient_text", "match_type"],
                transforms={"recipe_id": int, "ingredient_id": int, "quantity": real, "weight_gram": real},
            ),
            "recipe_instructions": import_rows(
                connection, "recipe_instructions.csv", ["recipe_id", "preparation_instructions"],
                "recipe_instructions", transforms={"recipe_id": int},
            ),
            "seasoner": import_rows(
                connection, "seasoner.csv", ["id", "name", "source_examples", "review_status"],
                "seasoner", transforms={"id": int},
            ),
            "recipes_seasoner": import_rows(
                connection, "recipes_seasoner.csv", ["recipe_id", "recipe_name", "seasoners"],
                "recipes_seasoner", columns=["recipe_id", "seasoners"], transforms={"recipe_id": int},
            ),
            "allergens": import_rows(
                connection, "allergens.csv", ["id", "code", "display_name"], "allergens", transforms={"id": int},
            ),
            "ingredient_allergens": import_rows(
                connection, "ingredient_allergens.csv",
                ["ingredient_id", "ingredient_name", "allergen_id", "allergen_name", "status", "note"],
                "ingredient_allergens", columns=["ingredient_id", "allergen_id", "status", "note"],
                transforms={"ingredient_id": int, "allergen_id": int},
            ),
            "dietary_tags": import_rows(
                connection, "dietary_tags.csv", ["id", "code", "display_name", "tag_group", "description"],
                "dietary_tags", transforms={"id": int},
            ),
            "ingredient_dietary_tags": import_rows(
                connection, "ingredient_dietary_tags.csv",
                ["ingredient_id", "ingredient_name", "dietary_tag_id", "dietary_tag_name", "status", "note"],
                "ingredient_dietary_tags", columns=["ingredient_id", "dietary_tag_id", "status", "note"],
                transforms={"ingredient_id": int, "dietary_tag_id": int},
            ),
        }
        for table, expected_count in counts.items():
            actual_count = scalar(connection, f"SELECT COUNT(*) FROM {table}")
            if actual_count != expected_count:
                raise RuntimeError(f"{table} count mismatch: expected {expected_count}, found {actual_count}")
        foreign_key_errors = connection.execute("PRAGMA foreign_key_check").fetchall()
        if foreign_key_errors:
            raise RuntimeError(f"foreign key validation failed: {foreign_key_errors}")
        integrity_result = connection.execute("PRAGMA integrity_check").fetchone()[0]
        if integrity_result != "ok":
            raise RuntimeError(f"integrity_check failed: {integrity_result}")
        connection.commit()
    except Exception:
        connection.rollback()
        raise
    finally:
        connection.close()

    os.replace(TEMP_DATABASE_FILE, DATABASE_FILE)
    return counts


if __name__ == "__main__":
    imported_counts = build_database()
    print(json.dumps({"database": str(DATABASE_FILE), **imported_counts}, indent=2))

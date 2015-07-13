#Item Definitions

##Example
```json
{ "file_type": "items"
, "definitions": [
    { "id": "WEAPON_SWORD_SHORT"
    , "name": "short sword"
    , "description": "A simple short sword."
    , "symbol": "/"
    , "symbol_color": "grey"
    , "weight": 2000
    , "tags": ["WEAPON", "WEAPON_EDGED"]
    }
  ]
}
```
##Fields
| Field        | Type     | Constraint | Optional | Description                                                               |
|:-------------|:---------|:-----------|:---------|:--------------------------------------------------------------------------|
| id           | string   | non-empty  | no       | The unique (after hashing) id string.                                     |
| name         | string   |            | yes      | The name displayed to the user.                                           |
| description  | string   |            | yes      | The description displayed to the user.                                    |
| symbol       | string   | non-empty  | no       | The visible glyph used as a textual representation.                       |
| symbol_color | string   |            | yes      | The color used for the glyph. White if omitted.                           |
| weight       | integer  | weight > 0 | no       | The weight of the item.                                                   |
| tags         | array    |            | yes      | An optional array of zero or more non-empty, unique (after hashing) tags. |

#Creature Definitions

##Example
```json
{ "file_type": "creatures"
, "definitions": [
  , { "id": "SKELETON"
    , "name": "skeleton"
    , "description": "A magically animated pile of bones."
    , "symbol": "s"
    , "symbol_color": "white"
    , "tags": ["UNDEAD", "EVIL"]
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
| symbol_color | string   |            | yes      | The color used for the glyph (white if absent).                           |
| tags         | array    |            | yes      | An optional array of zero or more non-empty, unique (after hashing) tags. |

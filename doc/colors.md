#Color Definitions

##Example
```json
{ "file_type": "colors"
, "definitions": [
    { "id": "WHITE"
    , "short_name": "w"
    , "value": [255, 255, 255]
    }
  ]
}
```
##Fields
| Field        | Type     | Constraint | Optional | Description                                                               |
|:-------------|:---------|:-----------|:---------|:--------------------------------------------------------------------------|
| id           | string   | non-empty  | no       | The unique (after hashing) id string.                                     |
| short_name   | string   |            | yes      | The name used to embed in other strings: "this is @w:white@/".            |
| value        | array    | size <= 4<br> integral elements n<br> n ∈ [0, 255]  | no       | The RGBA values (each set 255 if omitted).                                |

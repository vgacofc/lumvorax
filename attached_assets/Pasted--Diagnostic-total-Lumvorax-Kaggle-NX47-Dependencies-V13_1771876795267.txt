# Diagnostic total Lumvorax ↔ Kaggle (NX47 Dependencies V13)

## Contexte
Le rapport historique `lumvorax_360_validation_report_v6_binary.json` montrait `ok_with_warnings`, ce qui viole la certification "zéro warning".

## Problèmes racines confirmés

1. **Échec de chargement natif `.so`**
   - Erreur observée: `undefined symbol: neural_config_create_default`.
   - Impact: la dépendance native n'est pas réellement utilisable même si le fichier est présent.

2. **LZW non garanti au runtime Kaggle**
   - Erreur observée: `"<COMPRESSION.LZW: 5> requires the 'imagecodecs' package"`.
   - Cause typique: mismatch de wheel/tags Python runtime (ex: wheel non compatible runtime effectif).

3. **Validation précédente trop permissive**
   - Acceptait des warnings au lieu d'un échec bloquant.
   - Autres compressions (fallback) pouvaient masquer le vrai défaut LZW.

4. **Synchronisation versionnelle incomplète**
   - Le branding test/dataset était encore en V7.

## Correctifs appliqués (V13 strict réel)

1. **Validation par packages requis + compatibilité tags runtime**
   - Le validateur vérifie chaque package requis (`imagecodecs`, `numpy`, `tifffile`, etc.).
   - Chaque package doit avoir **au moins une wheel compatible** avec `packaging.tags.sys_tags()`.
   - Toute absence/incompatibilité requise déclenche `status=fail`.

2. **Installation offline sélectionnée sur wheel compatible**
   - La sélection n'est plus figée sur un seul nom de wheel.
   - Le validateur choisit automatiquement la meilleure wheel compatible trouvée dans le dataset.

3. **LZW durci sans fallback**
   - Vérifie explicitement `imagecodecs.lzw_encode` et `imagecodecs.lzw_decode`.
   - Si indisponible: échec immédiat.
   - `tifffile.imwrite(..., compression='LZW')` doit fonctionner réellement.

4. **`so_load` bloquant en strict**
   - `liblumvorax.so` doit se charger via `ctypes.CDLL`.
   - Sinon échec immédiat, aucun pass "avec warnings".

5. **Synchronisation V7 → V13 mise à jour**
   - Test kernel Kaggle renommé en `LUMVORAX V13 Certification Test`.
   - Dataset metadata renommée en `NX47 Dependencies V13`.

## Versions compatibles LZW / imagecodecs (règle fiable)

> Ce n'est pas un simple "nom de version" fixe: la compatibilité dépend du triplet **Python ABI + plateforme + manylinux tag**.

- `imagecodecs` doit être compatible avec les tags runtime Kaggle exposés par `sys_tags()`.
- Pour Python 3.12 Kaggle, préférer:
  - wheels `cp312-*` (ou `abi3` réellement supporté),
  - manylinux compatible avec l'image Kaggle utilisée.
- Même logique pour `numpy`, `pillow`, `scipy`, `scikit-image`.

## Critère d'acceptation final (zéro warning, zéro erreur)

- `status == "ok"`
- `warnings_count == 0`
- `so_load_status == "ok"`
- `roundtrip_status == "ok"`
- `missing_required_packages == []`
- `incompatible_required_packages == []`

## Plan de publication recommandé

1. Rebuild `liblumvorax.so` (sans symbole non résolu).
2. Publier dataset V13 avec wheels réellement compatibles tags Kaggle runtime.
3. Garder une seule version par package critique (éviter ambiguïtés).
4. Lancer le validateur V13 strict dans Kaggle et publier uniquement si verdict final est `ok` sans warning.


## Addendum traçabilité append-only — 2026-02-23T17:35:37Z

### Diagnostic complémentaire demandé (42 modules + TIFF/LZW + `.so`)
- Le validateur V13 a été renforcé avec un inventaire obligatoire de **42 modules** côté symboles exportés du `.so`.
- Chaque module est vérifié via un symbole requis; si un module manque, la validation échoue (`module_inventory_missing`).
- Le problème signalé `undefined symbol: neural_config_create_default` est maintenant explicitement dans l'inventaire obligatoire.
- Le pipeline TIFF LZW accepte désormais **plusieurs backends réels** (pas de placeholder):
  - backend `imagecodecs` (lzw_encode/lzw_decode),
  - backend `Pillow+libtiff` (fallback réel d'encodage TIFF LZW).

### Liste exacte des 42 modules certifiés (contrat minimal `.so`)
1. forensic_logger -> `forensic_logger_init`
2. forensic_logging -> `forensic_log_lum_operation`
3. log_manager -> `log_manager_create`
4. log_module -> `log_module_action`
5. lum_core -> `lum_create`
6. lum_content_text -> `lum_add_text_content`
7. lum_content_json -> `lum_add_json_content`
8. lum_content_image -> `lum_add_image_rgb24`
9. lum_content_som -> `lum_add_som_data`
10. lum_group_core -> `lum_group_create`
11. lum_group_ops -> `lum_group_add`
12. lum_group_sort -> `lum_group_sort_ultra_fast`
13. lum_group_defrag -> `lum_group_defragment_zero_copy`
14. lum_zone_core -> `lum_zone_create`
15. lum_zone_ops -> `lum_zone_add_group`
16. lum_memory_core -> `lum_memory_create`
17. lum_memory_store -> `lum_memory_store`
18. lum_memory_retrieve -> `lum_memory_retrieve`
19. lum_serialization_read -> `lum_read_serialized`
20. lum_serialization_write -> `lum_write_serialized`
21. lum_metadata_read -> `lum_read_metadata_serialized`
22. lum_metadata_write -> `lum_write_metadata_serialized`
23. lum_file_config -> `lum_file_config_create_default`
24. lum_file_context -> `lum_file_context_create`
25. lum_file_metadata -> `lum_file_metadata_create`
26. lum_file_result -> `lum_file_result_create`
27. lum_file_export -> `lum_export_single_binary`
28. lum_file_verify -> `lum_file_verify_integrity_file`
29. lum_universal_file -> `lum_universal_file_create`
30. lum_secure_serialize -> `lum_secure_serialize_group`
31. lum_secure_deserialize -> `lum_secure_deserialize_group`
32. lum_security_runtime -> `lum_security_init`
33. lum_security_checksum -> `lum_secure_calculate_checksum`
34. lum_logger_runtime -> `lum_logger_create`
35. lum_log_runtime -> `lum_log_init`
36. lum_log_analysis -> `lum_log_analyze`
37. vorax_core -> `vorax_create_node`
38. vorax_ops_fuse -> `vorax_fuse`
39. vorax_ops_split -> `vorax_split`
40. vorax_storage -> `vorax_store`
41. vorax_volume3d -> `vorax_volume3d_validate`
42. neural_default_config -> `neural_config_create_default`

### Point de vérité
- En environnement local actuel (hors Kaggle monté), les checks dataset/.so/roundtrip sont naturellement `skipped` quand le dataset n'est pas présent.
- La validation stricte complète doit être exécutée dans Kaggle avec le dataset monté pour conclure sur `status=ok` + `warnings=0`.

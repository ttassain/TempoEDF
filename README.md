# TempoEDF

* Affichage de la couleur du tarif EDF tempo
* Site web intégré pour consulter les infos a distance (couleur, jours restant pour chaque couleur)

## Hardware

- Espressif ESP32
- 2 x LED 3 couleurs

## Cablage

### LEDs

| GPIO | LED Aujourd'hui |
| --- | ----------- |
| 12 | Rouge |
| 13 | Vert |
| 14 | Bleu |

| GPIO | LED Demain |
| --- | ----------- |
| 25 | Rouge |
| 32 | Vert |
| 32 | Bleu |

## Author

TASSAIN Thierry

## License

TempoEDF is licensed under the Apache v2 License. See the LICENSE file for more info.
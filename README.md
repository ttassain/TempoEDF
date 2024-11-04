# TempoEDF

* Affichage de la couleur du tarif EDF tempo
* Site web intégré pour consulter les infos a distance (couleur, jours restant pour chaque couleur)

## Les sites qui m'ont aider a faire ce programme

* Site qui parle du sujet et dont j'ai pris du code : https://forum.arduino.cc/t/recuperation-des-couleurs-tempo-edf-avec-un-esp32/1198677

* Les 2 requêtes HTTP utilisées dans ce programme ont été créées  par J-M-L du forum francophone Arduino
   ici : https://forum.arduino.cc/t/api-rte-ecowatt/1017281

## API RTE

* https://digital.iservices.rte-france.com/open_api/tempo_like_supply_contract/v1
* https://data.rte-france.com/catalog/-/api/consumption/Tempo-Like-Supply-Contract/v1.1#

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
| 33 | Bleu |

## Author

TASSAIN Thierry

## License

TempoEDF is licensed under the Apache v2 License. See the LICENSE file for more info.
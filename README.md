# CROPS
C.R.O.P.S. : CRoissance Optimale des Plantes en Sol. Gestion des capteurs et stockage dans CouchDB

Colecte des donnees environementales pour unbe croissance optimale des plantes
et stockage de ces donnees dans une base de donmnee de type couchDB

Le systeme est base sur les capteurs suivants:
 - Le capteur DHT22 permet la mesure de la temperature ambiante
   ainsi que du niveau d'humiditee ambiante
 - Le capteur CSMS V1.2 permet la mesure de la quantitee d'eau dans le sol.
   CSMS: 'Capacitive Soil Moisture Sensor'
 - Le captteur BMP085 permet la mesure de la pression et de la temperature
   (de maniere semblet'il plus precise aue le DHT mais je ne l'ai pas verifie)
 - Le capteur TSL2561 Permet de mesurer la lumiere recue par les plantes
 
 le tout controle par un microcontroleur de type ESP32 avec wifi integre le "LoLin d32-Pro" et utilisant un afichage e-Paper LOLIN_IL3897.
 
 main.cpp    : Contiens le code a flasher sur l'EPS32 et qui vas collecter les donnees
 crops.ipyndb: Contiens le notebook Jupyter permetant de visualiser et d'analyser les donnees
 
 pour pouvoir fonctioner il vous faudra soit avoir CouchDB instale sur votre ordinateur soit une DB en ligne de style CloudAnt dont la version Lite devrais etre sufisante: https://cloud.ibm.com/catalog/services/cloudant) 
 
 
Ce code peut etre utilise avec la Nano ferme disponible sur thingiverse: https://www.thingiverse.com/thing:3654094

![NanoFarm Picture](https://github.com/kolergy/CROPS/blob/master/IMG_20190528_093121088.jpg)

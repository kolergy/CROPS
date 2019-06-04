# CROPS
C.R.O.P.S. : CRoissance Optimale des Plantes en Sol. Gestion des capteurs et stockage dans CouchDB

## C'est Quoi?
Colecte des donnees environementales pour unbe croissance optimale des plantes
et stockage de ces donnees dans une base de donmnee de type couchDB

Le systeme est base sur les capteurs suivants:
 - Le capteur DHT22 permet la mesure de la temperature ambiante
   ainsi que du niveau d'humiditee ambiante
 - Le capteur CSMS V1.2 permet la mesure de la quantitee d'eau dans le sol.
   CSMS: 'Capacitive Soil Moisture Sensor'
 - Le captteur BMP085 permet la mesure de la pression et de la temperature
   (de maniere semblet'il plus precise aue le DHT mais je ne l'ai pas encore verifie)
 - Le capteur TSL2561 Permet de mesurer la lumiere recue par les plantes
 
 le tout controle par un microcontroleur de type ESP32 avec wifi integre le "LoLin d32-Pro" et utilisant un afichage e-Paper LOLIN_IL3897.
 
 - main.cpp       : Contiens le code a flasher sur l'EPS32 et qui vas collecter les donnees
 - cropsDB.ipyndb : Contiens le notebook Jupyter permetant de visualiser et d'analyser les donnees
 
## Pour quoi ?
Il y as deux idees deriere ce developement:
- Tester differents capteurs pour une agriculture plus digitale et voir ce que l'on peux tirer de l'analyse des donnees mais tout de meme a bas cout et partager avec tous les moyens de le metre en oeuvre (l'ensemble du systeme de captation des donnees coute moins de 50$)
- L'autre question est est'il possible de produire en appartement avec eclairage artificiel tout en emetant autant voire moins de CO2 que des produits issus du suipermarche.

Ce Developement c'est effectue au Fablab Artilect https://artilect.fr/ et as ete presente pour la premiere fois durant le Fablab Festival 2019 https://fablabfestival.artilect.fr/
 
## Coment l'installer?

### Instalation de la partie capture des donnees
pour pouvoir fonctioner il vous faudra soit avoir un acces a CouchDB soit instale sur votre ordinateur soit une en ligne de style CloudAnt dont la version Lite (Gratuite) devrais etre sufisante: https://cloud.ibm.com/catalog/services/cloudant) 

le code main.cpp est prevu pour etre utilise avec PlatformIO sur atom
* Il vous faudra installer Atom: https://atom.io/
* Ensuite platformIO: dans Atom aller dans settings > Pakages > et instaler "platformio-ide"
* Dans platformIO Home, aller dan libraries et instalez les libraries suivantes:
  * Adafruit BMP085 
  * Adafruit GFX
  * Adafruit Unified Sensor
  * Adafruit TSL2561
  * ArduinoJson (Par Benoit Blanchon)
  * DHTesp (Par Bernd Giesecke)
  * NTPClient (Par Fabrice Weinberg)
  * TimeLib (Par Paul Stoffregen)
  * La librarie LOLIN_EPD (Par Wemos) est aussi necessaire mais n'est pas disponible en installation automatique il faudra aller la chercher ici: https://github.com/wemos/LOLIN_EPD_Library et copier le repository dans le repertoire lib de votre projet.
* Modifier le main.cpp afin dy introduire vos parametres WiFi et CouchDB dans les definitions 
 
### Instalation de la partie analyse des donnees
a detailler...
 
Ce code peut etre utilise avec la Nano ferme disponible sur thingiverse: https://www.thingiverse.com/thing:3654094

![NanoFarm Picture](https://github.com/kolergy/CROPS/blob/master/IMG_20190528_093121088.jpg)

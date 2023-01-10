# PSR Semestral Work - Motor Control

Semestrální projekt Petr Kučera & Jan Tonner vypracován studenty v rámci předmětu [PSR](https://rtime.ciirc.cvut.cz/psr/cviceni/semestralka/).

## Zadání projektu

Cílem semestrálního projektu je vytvořit digitální řízení motoru. Program má za úkol snímat polohu motoru na prvním zařízení, pomocí UDP protokolu ji přenést na zařízení druhé, nastavit motor na druhém motoru do stejné polohy a zobrazit informace jako živý graf na webovém serveru.

## Popis řešení

První zařízení, které snímá polohu nazýváme **master**. Program tento mód pozná tak, že při spuštění mu je jako parametr IP adresy zadána adresa cílového (**salve**) zařízení. Program následně funguje tak, že neustále čte polohu motoru a tu odesílá pomocí UDP protokolu do cílového zařízení. Program je ukončen stisknutím klávesy `q` či `Q`.

Druhé zařízení nazýváme **slave**. To pracuje poněkud komplikovaněji. Program do toho módu přejde v případě že parametr IP adresy je roven prázdnému *stringu*. Následně se vytvoří struktury nutné pro fungování *tásků* a *spawnou* se následující *tasky*:
- **tWebServer** - Spustí webový server, který čeká na dotazy zařízení a následně tvoří nové dílčí *tasky*, které mají za úkol odpovědět na *HTTP Request*.
  - pracuje se strukturou `HTTP_D`
- **tUDPHandler** - Zpracovává pakety přicházející pomocí UDP protokolu a ukládá pozici motoru na *master* zařízení do proměnné `wanted_position`, která je v `UDP` struktuře.
  - pracuje se strukturou `UDP`
- **tHTTPDHandler** - Vzorkuje data po 2 ms a ukládá je do struktury `HTTP_D`.
  - pracuje se strukturami `UDP`, `psrMotor` a `HTTP_D`
- **tMotorControl** - Čte data z proměnné `wanted_position`, která je v `UDP` struktuře a snaží se otočit motorem do dané pozice.
  - pracuje se strukturami `UDP` a `psrMotor`

Jednotlivé tasky běží dokud nejsou stejně jako v předchozím módu ukončeny stisknutím klávesy `q` či `Q`.

## Přílohy

Workflow data je zobrazen ve schématu [`data_chart.drawio`](data_chart.drawio), který si je možno zobrazit např. s rozšířením [Drawio Preview](https://marketplace.visualstudio.com/items?itemName=purocean.drawio-preview) ve [VS Code](https://code.visualstudio.com/).

## Dokumentace

Dookumentaci je možné vygenerovat automaticky pomocí Doxygen.

```sh
sudo apt install doxygen # install doxygen to creating a documentation
doxygen # generate documentation
```
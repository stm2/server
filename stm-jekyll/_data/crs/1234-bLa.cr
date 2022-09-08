VERSION 69
"UTF-8";charset
"de";locale
1;noskillpoints
1659958813;date
"eressea";Spiel
"Java-Tools";Konfiguration
"Hex";Koordinaten
36;Basis
1;Umlaute
0;curTempID
1234;Runde
2;Zeitalter
"eressea-server@kn-bremen.de";mailto
"ERESSEA 2 BEFEHLE";mailcmd
15022;reportowner
"27.1.2-2-g5645d9678";Build
2500;max_units
COORDTRANS 15022
0 0;translation
PARTEI 15022
"de";locale
599;Optionen
"Orks";Typ
70;Rekrutierungskosten
1;Anzahl Personen
1;age
"gray";Magiegebiet
"Partei bLa";Parteiname
"max.mustermann@example.com";email
OPTIONEN
1;REPORT
1;COMPUTER
1;ZUGVORLAGE
0;SILBERPOOL
1;STATISTIK
0;DEBUG
1;ZIPPED
0;ZEITUNG
0;MATERIALPOOL
1;ADRESSEN
0;BZIP2
0;PUNKTE
0;TALENTVERSCHIEBUNGEN
MESSAGE 971112208
198804487;type
"Deine Partei hat letzte Runde keinen Zug abgegeben!";rendered
MESSAGE 1063819696
771334452;type
"Einheit 5ac0 (5ac0) verdient in Rilmodciget (0, 0) 10 Silber.";rendered
246672;unit
0 0 0;region
10;amount
10;wanted
0;mode
MESSAGE 761101056
1784377885;type
"Das Passwort für diese Partei lautet keinpasswort";rendered
"keinpasswort";value
MESSAGE 751019600
1593006007;type
"Deine Partei ist noch die nächsten 6 Wochen immun gegen Angriffe.";rendered
6;turns
REGION -1 0
6231171415;id
"Ozean";Terrain
"neighbour";visibility
REGION 0 -1
943437574;id
"Ozean";Terrain
"neighbour";visibility
REGION 0 0
1102578064;id
"Gusras";Name
"Wald";Terrain
"";Beschr
500;Baeume
100;Schoesslinge
4004;Bauern
51;Pferde
83960;Silber
4198;Unterh
100;Rekruten
11;Lohn
1;aktiveRegion
RESOURCE 347593380
"Bauern";type
4004;number
1234;Runde
RESOURCE 6035652
"Bäume";type
500;number
1234;Runde
RESOURCE 1352714618
"Schößlinge";type
100;number
1234;Runde
RESOURCE 210060480
"Silber";type
83960;number
1234;Runde
RESOURCE 200695649
"Pferde";type
51;number
1234;Runde
PREISE
8;Balsam
25;Gewürz
-7;Juwel
20;Myrrhe
12;Öl
24;Seide
20;Weihrauch
EINHEIT 246672
"Einheit 5ac0";Name
15022;Partei
1;Anzahl
"Orks";Typ
0;Kampfstatus
32500;weight
COMMANDS
"; Befehle, die mit Semicolon beginnen, sind Kommentare."
"; Die Standardeinstellungen von Magellan sorgen dafür, dass sie vor dem "
"; Versand an den Server entfernt werden. Magellan sorgt auch dafür, dass zu lange Zeilen wie diese hier umgebrochen werden und auf dem Server keine Probleme machen."
"; Wir nutzen die Kommentare hier, um dir die Befehle zu erklären."
"; "
"; Wir wollen eine schöne, passende Nummer (id) für unsere Partei sichern."
"; IDs Bestehen aus einer Kombination von 4 Zahlen oder Buchstaben."
"; Die IDs für Parteien, Einheiten, Gebäuden und Schiffen kommen jeweils"
"; aus einem eigenen Pool."
"NUMMER PARTEI demo"
"; Es ist also möglich dieser Einheit die gleiche Nummer wie die der Partei"
"; zu geben"
"NUMMER EINHEIT demo"
"; Zurück zur Partei. Wir wollen unserer Partei einen guten Namen und eine"
"; hübsche Beschreibung geben. Eressea spielt man mit anderen zusammen."
"; Schöne Beschreibungen erhöhen den Spielspaß für alle und vermindern die "
"; Wahrscheinlichkeit von seinen Nachbarn früh abgeschlachtet zu werden. "
"; Wirklich wahr."
"BENENNE PARTEI \"Die DEMOkraten\""
"BANNER \"Wir glauben an demokratische Werte. Sie sind der beste Weg für ein gutes Zusammenleben. Wenn ihr das auch denkt, schreibt uns doch eine Nachricht an demo@example.com!\""
"BENENNE EINHEIT \"Erster DEMOrKRAT\""
"BESCHREIBE EINHEIT \"Tritt für Gewaltenteilung ein. Also zumindest für Gewalt. Und Teilung.\""
"; "
"; Wir ändern noch rasch den Kampfstatus. Im Moment ist das streng genommen"
"; nicht nötig, aber so vergessen wir es später nicht. Für Kundschafter und"
"; andere Unbewaffnete ist FLIEHE eine gute Wahl: für den Fall, dass wir"
"; von unliebsamen Nachbarn oder Monstern angegriffen werden, können wir uns"
"; noch nicht wirksam verteidigen. Stattdessen vergrößern wir unsere Chancen"
"; zu überleben UND verlieren außerdem unseren Bewegungsbefehl nicht."
"; Win-Win!"
"KÄMPFE FLIEHE"
";"
"; Der Rest der Befehle dieser Einheit besteht nur aus Übergaben an"
"; TEMP-Einheiten und der Erzeugung von TEMP-Einheiten."
"; Gelogen! Wir sichern uns noch schnell Silber, um wenigstens die nächsten"
"; 4 Runden überleben zu können."
"RESERVIERE 40 Silber"
";"
"; Überhaupt Silber. Wir haben 2500 und so bald stehen keine Einnahmen ins"
"; Haus. Wir sind schließlich keine Halblinge, die ab Runde zwei fette"
"; Einnahmen haben, weil die Bauern sie sooo lustig finden und ihnen"
"; freiwillig Geld in den ... Ich schweife ab. "
"; Was können wir tun?"
"; 2500 Silber, also rekrutieren wir 35 Orks zu je 70 Silber, richtig?"
"; Falsch natürlich, das führt in die sichere Katastrophe: Wir brauchen ja"
"; auch 10 Silber Unterhalt pro Runde."
"; Unsere ersten Einnahmen stehen frühestens in Runde 3 an: "
"; Runde 1: Einheit 1 lernt Waffenbau"
"; Runde 2: Einheit 1 macht Holz zu Speeren, Einheit 2 lernt Treiben"
"; Runde 3: Einheit 2 schappt sich die Speere und treibt. Bis dahin haben"
"; sie wahrscheinlich nur je eine Talentstufe gelernt, dazu kommt unser "
"; Rassenbonus von +1 auf Steuereintreiben."
"; Macht also 40 Silber pro Runde."
"; Effizienter ist es wahrscheinlich Folgendes: Wir lassen eine Person "
"; Steuereintreiben lernen. Die benutzen wir in der nächsten Runde als"
"; Lehrer für unsere ersten 10 Treiber."
"; Durch diesen Trick sparen wir das Unterhaltssilber für die 10 Treiber in"
"; der ersten Runde beim gleichen Ergebnis. Vielleicht ist es auch"
"; sinnvoller, sie dann gleich noch eine Runde lernen zu lassen. Auf diese"
"; Weise werden sie wahrscheinlich Stufe 2 erreichen und können dann gleich"
"; 60 Silber pro Runde machen. Bis dahin braucht jede Person also 30 Silber"
"; Unterhalt. Genaues Rechnen lohnt sich. Ist es sinnvoll, die"
"; Steuereintreiber trotzdem schon diese Runde zu rekrutieren und etwas;"
"; anderes lernen zu lassen?"
";"
"; Wir übergeben das Rekrutierungssilber plus drei Runden Unterhalt. Das ist"
"; nicht unbedingt nötig, weil die Einheit sich das Silber auch aus dem "
"; Silberpool nimmt."
"; Aufgrund der Übersichtlichkeit ist es aber eine gute Praxis."
"; Orks sind billig, wir können also wenigsten viel mehr Einheiten "
"; rekrutieren als zum Beispiel Elfen. Bäh!"
"; 1 Treiberlehrer"
"GIB TEMP de00 100 Silber"
"; 3 oder 4 Waffenbauer"
"GIB TEMP de01 400 Silber"
"; 1 Tarner"
"GIB TEMP de02 100 Silber"
"; 1 Wahrnehmer"
"GIB TEMP de03 100 Silber"
"; Unsere Späher bekommen neben dem Rekrutierungssilber noch 50 Silber,"
"; um als Kundschafter 5 Runden durch die Lande ziehen zu können. Plus 120"
"; Silber, um einen weiteren Kundschafter in der Nachbarregion rekrutieren"
"; zu können."
"GIB TEMP de04 240 Silber"
"GIB TEMP de05 240 Silber"
"; weitere Einheiten"
"GIB TEMP de06 100 Silber"
"GIB TEMP de07 100 Silber"
"GIB TEMP de08 200 Silber"
"; Dies ist der letzte Befehl. Wir übergeben alles, was wir noch haben, an"
"; unser Depot."
"; Wir behalten nur reservierte Gegenstände. Das ist eine gute Angewohnheit,"
"; um nicht aufgrund von Denkfehlern überladen zu werden."
"GIB TEMP de02 ALLES"
"; Aha! Wir haben selber noch keinen langen Befehl bekommen. Nun, da wir"
"; schon einen Talentvorsprung haben, können wir als Ausbilder für unsere"
"; zukünftigen Treiber / Krieger dienen."
"; Die Frage ist nur, ob wir Hieb- oder Stangenwaffen lernen"
"LERNE AUTO Hiebwaffen"
"; Das war's! Wir könnten noch Pferdedresseure (super wichtig für;"
"; Steintransport und den Krieg), Segler und Schiffbauer (schließlich haben"
"; wir 4 Ozeanregionen) anstellen."
"; Hätten wir keinen Malus auf Magie, würden wir wahrscheinlich auch 1 oder"
"; 4 Magier ausbilden. Aber die brauchen bei uns drei Wochen und 500 Silber,"
"; bis sie auch nur Stufe 1 erreicht haben. Wir warten lieber. Das hat"
"; wenigstens den Vorteil, dass wir uns noch länger Zeit lassen können, ein"
"; Magiegebiet zu wählen."
";"
"; Wir belassen es dabei. Wir haben schon 14 Personen und jede weitere"
"; verlangsamt unseren Start, da wir weniger Steuereintreiber einsetzen"
"; können. Dafür bleiben uns jetzt noch etwa 950 Silber."
"; "
"; Allzu oft fragt man sich: Was habe ich mir letzte Runde wohl dabei"
"; gedacht? //-Kommentare erscheinen nächste Woche im Report und sind"
"; deshalb ein guter Weg, um Pläne über mehrere Wochen nicht zu vergessen."
"// Plan für Woche 1235:"
"// 10 Steuereintreiber rekrutieren und lehren lassen"
"// Speere bauen"
"// Plan für Woche 1236:"
"// Treiber lernen, mehr Speere"
"// Plan für Woche 1237:"
"// Profit!"
"; Vielleicht ist das noch nicht der beste Plan. Es lohnt sich, gerade in"
"; den ersten Runden alles Schritt für Schritt vorauszudenken. Wie genau"
"; sieht die Situation nächte Woche aus?"
"; Wie in 2, 3 oder 10 Wochen? Wir wollen dann nicht merken, dass wir etwas"
"; Wichtiges versäumt haben!"
";"
"; Die Befehle MACHE TEMP, ENDE zur Erzeugung von TEMP-Einheiten, sowie den"
"; Befehl NÄCHSTER am Ende unserer Befehle fügt Magellan netterweise "
"; selbständig hinzu. Achte darauf, dass du vor dem Absenden das richtige"
"; Passwort einträgst."
"MACHE TEMP de00"
"; Unser zukunftüger Lehrer"
"BENENNE EINHEIT \"DEMOrkratieverwalter\""
"; Haben wir an das Silber zum Rekrutieren gedacht? Haben wir."
"; Magellan würde auch eine Warnung in der Offene-Probleme-Ansicht geben,"
"; falls wir es vergessen."
"REKRUTIERE 1"
"; Durch LERNE AUTO müssen wir nicht von Hand Lehrer auf Schüler verteilen."
"; Wir können theoretisch nächste Runde nur 8 Steuereintreiber rekrutieren"
"; und mit LERNE AUTO Steuereintreiben lernen lassen (jedoch nicht einfach"
"; LERNE Steuereintreiben!)"
"; Die 2 \"fehlenden\" Schüler nutzt unser Lehrer dann, um selber zu lernen."
"LERNE AUTO Steuereintreiben"
"ENDE"
"MACHE TEMP de01"
"// Runde 1235: mit T3 6 Speere pro Runde bauen"
"BENENNE EINHEIT \"Kunsthandwerker\""
"REKRUTIERE 4"
"LERNE AUTO Waffenbau"
"ENDE"
"MACHE TEMP de02"
"; Eine Depoteinheit, die alle unsere nicht benötigten Gegenstände bekommt"
"; und Tarnung lernt. Auf diese Weise verringern wir die Informationen, die"
"; feindliche Späher über unsere Partei bekommen."
"; Gleichzeitig haben Einheiten mit Tarnung eine bessere Fluchtchance, falls"
"; wir überfallen werden."
"; Vor dem Beklautwerden schützt dies leider nicht, weil blöde Goblindiebe"
"; aus dem Silberpool klauen."
"BENENNE EINHEIT \"DEpot\""
"REKRUTIERE 1"
"; Tarnung und Wahrnehmung sind mit die wichtigsten Talente. Deshalb lohnt"
"; es sich wahrscheinlich, sie ab Runde 1 zu lernen!"
"LERNE AUTO Tarnung"
"ENDE"
"MACHE TEMP de03"
"; Wir lernen auch Wahrnehmung ab Runde 1. Wir möchten ja nicht von Goblins"
"; und Katzen totgeklaut werden!"
"BENENNE EINHEIT \"Wächter der Demokratie\""
"REKRUTIERE 1"
"LERNE AUTO Wahrnehmung"
"ENDE"
"MACHE TEMP de04"
"; Wir wollen uns so früh wie möglich unsere Umgebung anschauen. "
"; Der Berg im Osten scheint uns besonders interessant. Dahinter wird es"
"; hoffentlich weitere Regionen geben, also wollen wir dort nächste Runde"
"; einen zweiten Botschafter rekrutieren."
"BENENNE EINHEIT \"Botschafter der DEMOkratie\""
"BESCHREIBE EINHEIT \"Der Botschafter trägt fröhliche schwarz-rot-goldene Klamotten\""
"REKRUTIERE 1"
"NACH o"
"ENDE"
"MACHE TEMP de05"
"; Die nächsten 2 Botschafter schicken wir nach Südosten"
"BENENNE EINHEIT \"Botschafter der DEMOkratie\""
"BESCHREIBE EINHEIT \"Der Botschafter trägt fröhliche schwarz-rot-goldene Klamotten\""
"REKRUTIERE 1"
"NACH so"
"ENDE"
"MACHE TEMP de06"
"; Das ist wichtig. Für die meisten Völker ist Unterhaltung das bessere"
"; Talent, um die Wirtschaft aufzubauen. Wir haben aber -2 auf Unterhaltung,"
"; also kostet es uns 6 Wochen, überhaupt funktionsfähige Unterhalter"
"; auszubilden. Steuereintreiber brauchen thoeretisch nur eine Woche:"
"; eine für Steuereintreiben, das Waffentalent bringen sie schon mit. Nur "
"; brauchen sie dann noch eine Waffe."
"; Am Anfang werden das Speere sein. Holz ist super knapp in Eressea. In "
"; der Region gibt es 500 Bäume und 100 Schößlinge. Also ist unser Plan,"
"; schnell Eisen zu finden, damit ein Sägewerk zu bauen, so dass wir daraus"
"; 1200 Holz machen können. Das reicht für bis zu 1200 Treiber und Krieger,"
"; um uns Respekt zu verschaffen. Die Produktion dauert aber. Hoffentlich"
"; sind wir bis dahin nicht von einem aggressiven Insekten- oder einem"
"; boomenden Halblingsvolk überrannt worden!"
"; Schritt 1 ist: Eisen finden für das Sägewerk."
"; Vielleicht gibt es Eisen oder Stein in unserer Startregion. Sonst müssen"
"; wir den Berg ausbeuten und hoffen, dass sich dort noch kein Z ... kein Zw"
"; ... Boah, das kommt mir schwer über die Lippen ... Keiner unserer"
"; vertikal benachteiligten Zeitgenossen breit gemacht hat."
"; Mittel- und langfristig sollten wir nicht auf Stangenwaffen, sondern auf"
"; Hiebwaffen setzen."
"; Die sind fast immer besser bei gleichen Ressourcen und wir können unser"
"; Holz für Gebäude, Schiffe und Wagen benutzen."
"BENENNE EINHEIT \"Kumpel\""
"REKRUTIERE 1"
"LERNE AUTO Bergbau"
"ENDE"
"MACHE TEMP de07"
"; Steine sind auch wichtig für Handelsposten (okay, am Anfang eher nicht"
"; für uns), Burgen zur Verteidiung und Sägewerke, Bergwerke und"
"; Steinbrüche."
"BENENNE EINHEIT \"Kumpel\""
"REKRUTIERE 1"
"LERNE AUTO Steinbau"
"ENDE"
"MACHE TEMP de08"
"; Wir rekrutieren auch noch ein paar Holzfäller, damit unsere Waffenbauer"
"; auch in Zukunft was zu tun haben."
"BENENNE EINHEIT \"Landschaftspfleger\""
"REKRUTIERE 2"
"LERNE AUTO Holzfällen"
"ENDE"
TALENTE
300 4;Armbrustschießen
300 4;Bogenschießen
300 4;Katapultbedienung
300 4;Hiebwaffen
300 4;Stangenwaffen
GEGENSTAENDE
10;Holz
2500;Silber
4;Stein
REGION 1 -1
6811631996;id
"Gytsosuncun";Name
"Wüste";Terrain
"neighbour";visibility
REGION 1 0
5061153169;id
"Tanpeldoddod";Name
"Berge";Terrain
"neighbour";visibility
REGION -1 1
233888444;id
"Ozean";Terrain
"neighbour";visibility
REGION 0 1
6874843160;id
"Ozean";Terrain
"neighbour";visibility
TRANSLATION

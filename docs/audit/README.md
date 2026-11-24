# Audit de navigation — Music Player sur TrimUI Brick

- **Date** : 2026-11-23
- **Console** : TrimUI Brick (`tg5040`), branchée en ADB
- **Version** : `v1.20.0-beta.2+local`, branche `feat/audiobook` (`0458e4a`), build debug
- **Méthode** : l'app est lancée depuis NextUI avec le canal de contrôle (`--test-control` sur un FIFO).
  Chaque étape envoie une touche, attend, puis prend une capture d'écran sur la console
  (`screenshot()`), rapatriée dans `screens/`.
- **Données** : la carte SD de la console telle quelle (15 morceaux dans `/Music`, une playlist `ss`,
  6 livres dans `/Audiobook`, aucun podcast abonné, une station de radio par défaut).

> Les captures sont en 1024×768, la résolution de l'écran de la Brick.

## Parcours

| # | Capture | Action | Ce qu'on voit |
|---|---|---|---|
| 01 | `01-menu-principal.png` | lancement | Menu principal. Une ligne `Resume: Moustache Gracias` en tête, puis Library, Audiobook, Online Radio, Podcasts, Settings |
| 02 | `02-aide-menu-principal.png` | START | Aide du menu principal, en dialogue sur le menu assombri |
| 04 | `04-files.png` | Library → Files | Explorateur de `/Music` : dossiers puis fichiers MP3, icône de format |
| 05 | `05-aide-files.png` | START | Aide de l'explorateur, plein écran (pas de fond visible) |
| 06 | `06-files-curseur.png` | Bas ×2 | Curseur sur un morceau |
| 07 | `07-lecteur-musique.png` | A | Lecteur musique : pochette en fond, artiste, titre, album, temps, indicateurs LYRICS / SHUFFLE / REPEAT |
| 08 | `08-aide-lecteur-musique.png` | START | Aide du lecteur musique |
| 09 | `09-lecteur-musique-spectre.png` | L2 | Spectre activé en bas d'écran |
| 10 | `10-lecteur-musique-paroles.png` | R2 | Paroles synchronisées, ligne courante en blanc |
| 11 | `11-lecteur-musique-shuffle-repeat.png` | X puis Y | SHUFFLE et REPEAT soulignés (actifs) — remis à off ensuite |
| 12 | `12-lecteur-musique-piste-suivante.png` | Haut | Piste suivante (02/15), nouvelle pochette |
| 13 | `13-ecran-eteint-message.png` | SELECT | Écran noir « Press SELECT + A to wake screen » |
| 14 | `14-ecran-reveille.png` | SELECT + A | Retour au lecteur, la lecture continue |
| 15 | `15-files-apres-lecture.png` | B | Retour à l'explorateur, la musique continue en fond |
| 16 | `16-playlists.png` | Library → Playlists | Liste des playlists : `ss (2)` |
| 17 | `17-aide-playlists.png` | START | Aide : Y nouvelle playlist, X suppression |
| 18 | `18-playlist-detail.png` | A | Détail de la playlist : 2 morceaux |
| 19 | `19-aide-playlist-detail.png` | START | Aide : X retire un morceau |
| 20 | `20-lecture-playlist.png` | A | Lecture depuis la playlist (01/02) |
| 21 | `21-menu-now-playing-music.png` | B ×4 | Menu principal : la ligne devient `Now Playing: Music` |
| 22 | `22-audiobook-liste.png` | Audiobook | Liste des livres : ligne Continue, livres en cours (chapitre + position), terminés, durée totale des autres |
| 23 | `23-aide-audiobook-liste.png` | START | Aide de la liste Audiobook |
| 24 | `24-audiobook-liste-curseur.png` | Bas | Le titre de la ligne Continue est visible en entier une fois le curseur parti |
| 25 | `25-audiobook-reprise.png` | Continue → A | Lecteur Audiobook juste après la reprise (chapitre 18/41) |
| 26 | `26-audiobook-lecteur.png` | attente 4 s | Lecteur Audiobook : pochette, titre du chapitre qui défile, progression du chapitre, temps restant, position dans le livre |
| 27 | `27-aide-audiobook-lecteur.png` | START | Aide du lecteur Audiobook |
| 28 | `28-audiobook-pause.png` | A | Pause : « Paused » dans la ligne d'état, l'indice passe à `A PLAY` |
| 29 | `29-audiobook-chapitres.png` | X | Liste des chapitres, curseur sur le chapitre en cours (marqué `>`) |
| 30 | `30-aide-audiobook-chapitres.png` | START | Aide de la liste des chapitres |
| 31 | `31-audiobook-avance-30s.png` | B, Droite | +30 s |
| 32 | `32-audiobook-minuterie-15.png` | Y | Toast « Sleep timer: 15 min », compte à rebours dans la ligne d'état |
| 33 | `33-audiobook-minuterie-fin-chapitre.png` | Y ×4 | « Sleep timer: end of chapter » |
| 34 | `34-audiobook-minuterie-off.png` | Y | Minuterie désactivée |
| 35 | `35-audiobook-ecran-eteint.png` | SELECT | Écran noir de mise en veille |
| 36 | `36-audiobook-ecran-reveille.png` | SELECT + A | Réveil — **mais le livre est passé en pause** (voir constats) |
| 37 | `37-audiobook-retour-liste.png` | B | Retour à la liste, progression mise à jour (02:18) |
| 38 | `38-menu-apres-audiobook.png` | B | Menu principal : `Resume: Moustache Gracias`, pas de `Now Playing: Audiobook` (livre arrêté car en pause) |
| 39 | `39-radio-liste.png` | Online Radio | Une station par défaut, note « These are default stations » |
| 40 | `40-aide-radio.png` | START | Aide de la liste radio |
| 41 | `41-radio-lecture.png` | A | Lecteur radio : « Failed to fetch playlist », état `error` |
| 42 | `42-aide-radio-lecture.png` | START | Aide du lecteur radio |
| 43 | `43-radio-retour-liste.png` | B | Retour à la liste |
| 44 | `44-podcasts.png` | Podcasts | État vide « No podcasts subscribed » |
| 46 | `46-podcasts-gestion.png` | Y | Manage Podcasts : Search, Top Shows |
| 47 | `47-podcasts-top-shows.png` | Top Shows | « Failed to fetch charts » |
| 48 | `48-settings.png` | Settings | Réglages : Auto Screen Off, Bass Filter, Soft Limiter, Updates… |
| 49 | `49-aide-settings.png` | START | Aide des réglages |
| 50 | `50-settings-bas.png` | Haut (boucle) | Bas de la liste : About, Input test |
| 51 | `51-about.png` | About | Version, QR code, « Failed to check GitHub » |

Les numéros manquants (03, 45, 52) sont des captures ratées, supprimées.

## Constats

### Bugs (Audiobook, branche en cours)

1. **Le réveil met le livre en pause** (36, 38). Avec Select + A sur l'écran noir, le A qui réveille
   déclenche aussi Lecture/Pause. Ensuite, B sur un livre en pause arrête la session, donc le menu
   principal n'affiche pas `Now Playing: Audiobook`. Le lecteur musique (14) n'a pas ce défaut.
2. **L'indice `A PLAY` manque en bas de la liste Audiobook** (22, 24) : seuls `X FINISHED` et
   `B BACK` s'affichent. La barre ne tient que deux paires d'indices, la troisième est perdue.
3. **Aide tronquée** (23) : « Mark Finished/Unfini… ».
4. **Titre de chapitre surdimensionné** (25–36) : la police « titre » dans la demi-largeur à droite
   de la pochette fait défiler presque tous les titres (« 18 - Chapitre 4 »). La ligne d'état est
   aussi tronquée avec la minuterie de fin de chapitre (« Sleep end … », 33).
5. **Auteur vide pour un livre en dossier de MP3** (26) : l'espace de l'auteur reste blanc. Les tags
   ID3 (artiste) ne sont pas lus pour ces livres, seuls les `.m4b` le sont.

### Incohérences entre écrans

- **Deux générations d'interface cohabitent.** Les menus et listes en pages natives (menu principal,
  Library, Audiobook, Radio, Settings, About) ont une barre d'indices complète et des aides en
  dialogue sur l'écran assombri (02, 23). Les écrans legacy (Files, Playlists, lecteurs) ont des
  aides en plein écran sur fond noir (05, 08, 17, 19, 42).
- **Les lecteurs ne se ressemblent pas.** Le lecteur musique (07) a une pochette en fond, une puce
  de format, un compteur de pistes et pas d'indices de touches. Le lecteur radio (41) n'a pas
  d'indices non plus. Le lecteur Audiobook (26) a une pochette en vignette, la barre de titre
  standard et des indices. Trois mises en page pour la même fonction.
- **Vocabulaire des indices** : `A OPEN` (menus), `A SELECT` (Files, Playlists), `A PLAY`
  (Radio, chapitres) pour des actions semblables.
- **Reprise** : la musique a une ligne `Resume` dans le menu principal, l'Audiobook a une ligne
  `Continue` dans son propre menu. Deux mots et deux endroits pour la même idée.
- **Couleurs des états d'erreur** : rouge pour la radio et About (41, 51), blanc pour Top Shows (47).
- **Langue** : l'interface est en anglais, mais les contenus (titres, chapitres) sont en français.
  Rien à corriger dans l'app, mais à garder en tête pour une audience d'enfants francophones.

### Autres remarques

- **Explorateur** (15) : le morceau en cours de lecture n'est pas signalé dans la liste.
- **Réseau** : toutes les fonctions en ligne échouent (radio 41, Top Shows 47, GitHub 51) alors que
  le Wi-Fi est affiché connecté. À vérifier côté console (accès internet, heure système, certificats)
  avant de conclure à un bug de l'app.
- **Settings** (50) : maintenir Bas ne fait pas défiler la liste. Haut boucle jusqu'au dernier élément.
- **État laissé sur la console** : le spectre (L2) et les paroles (R2) ont été activés pendant
  l'audit, et la position d'Ewilan II a avancé d'environ 1 min 20 (02:18 au lieu de 00:55).

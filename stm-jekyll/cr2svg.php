<?php

if (empty($argv[1]) || empty($argv[2])) {
  echo "\n    Usage: $argv[0] input.cr output.svg\n\n";
  return;
}

$inputName = $argv[1];
$outputName = $argv[2];

$myfile = fopen($inputName, "r") or die("Unable to open file!");

$mysvg = fopen($outputName, "w");

$preg_region = '/^REGION (-{0,1}[0-9]+) (-{0,1}[0-9]+)( (-{0,1}[0-9]+)$){0,1}$/';
$preg_terrain = '/^"(.*)";Terrain$/';

function parse_region($line, $matches) {
    $parts = preg_split("/ /", $line);
    $region = array();
    $region['x'] = $matches[1]-1500;
    $region['y'] = $matches[2]-1500;
    if (!empty($matches[3]))
      $region['z'] = $matches[3];
    $region['type'] = null;
    return $region;
}

$colors = array(
    'default' => 'grey',
    'Ozean' => '#0000ff',
    'Ebene' => '#ffff00',
    'Wald' => '#00dd00',
    'Sumpf' => '#226611',
    'Berge' => '#777777',
    'Hochland' => '#ffeeaa',
    'Wüste' => 'ffcc55#',
    'Gletscher' => '#bbbbcc',
    'Eisberg' => '#eeeeff',
    'Vulkan' => '#bb0022',
    'Aktiver Vulkan' => '#ee0022',
    'Feuerwand' => '#ff0000',
);

$images = array(
    'default' => 'region',
    'Ozean' => 'ozean',
    'Ebene' => 'ebene',
    'Wald' => 'wald',
    'Sumpf' => 'sumpf',
    'Berge' => 'berge',
    'Hochland' => 'hochland',
    'Wüste' => 'wueste',
    'Gletscher' => 'gletscher',
    'Eisberg' => 'eisberg',
    'Vulkan' => 'vulkan',
    'Aktiver Vulkan' => 'aktiver vulkan',
    'Feuerwand' => 'feuerwand',
    'Nebel' => 'nebel',
    'Dichter Nebel' => 'dichter nebel',
    'Packeis' => 'packeis',
    'Gang' => 'gang',
    'Halle' => 'halle',
    'Wand' => 'wand'
);

function get_color($terrain) {
    global $colors;
    if (empty($colors[$terrain]))
      return $colors['default'];
    else {
        return $colors[$terrain];
    }
}

function get_image($terrain) {
    global $images;
    if (empty($images[$terrain]))
        return $images['default'];
    else {
        return $images[$terrain];
    }
}


$front_matter = <<<EOT
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg
    xmlns:cc="http://creativecommons.org/ns#"
    xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:svg="http://www.w3.org/2000/svg"
    xmlns="http://www.w3.org/2000/svg"
    xmlns:xlink="http://www.w3.org/1999/xlink"
    width="%d" height="%d"
    viewBox="%d %d %d %d">
  <defs>
  <g id='region'>
  <polygon points="50 0,100 25,100 75, 50 100,0 75,0 25"
           stroke="black"
           stroke-width="1" />
  </g>

EOT;
$back_matter=<<<EOT

</g>
</svg>
EOT;

$use_tag = '<use xlink:href="#%s" x="%d" y="%d" fill="%s" />' . "\n";

$rwidth = 100;
$yoff = $rwidth * .5;

function include_image($image) {
    global $mysvg;
    $filename = "images/$image.svg";
    if (file_exists($filename)) {
        $contents = file_get_contents($filename);
        $contents = preg_replace('|^.*<svg.*(<image.*/>).*</svg>.*$|s', "\n".'$1'."\n", $contents);
        fwrite($mysvg, "\n<g id='$image'>\n");
        fwrite($mysvg, $contents);
        fwrite($mysvg, "\n</g>\n");
    } else {
        warn("$filename not found.");
    }
}

function output_front() {
    global $mysvg, $front_matter, $bounds, $images;
    fwrite($mysvg, sprintf($front_matter,
        ($bounds['xmax'] - $bounds['xmin'] + 110) / 2,
        ($bounds['ymax'] - $bounds['ymin'] + 110) / 2,
        $bounds['xmin'] - 5, $bounds['ymin'] -5,
        $bounds['xmax'] - $bounds['xmin'] + 110, $bounds['ymax'] - $bounds['ymin'] + 110));

    foreach ($images as $terrain => $image) {
        include_image($image);
    }

    fwrite($mysvg, "  </defs>\n      <g>\n");
}

function output_back() {
    global $mysvg, $back_matter;
    fwrite($mysvg, $back_matter);
}

function transformx($region) {
    global $rwidth, $yoff;
    return round($region['x'] * $rwidth + $region['y'] * $yoff);
}

function transformy($region) {
    global $rwidth;
    return round($region['y'] * - $rwidth * 3 / 4);
}

function output_region($region) {
    global $mysvg, $use_tag, $bounds;
    if (empty($region['z'])) {
        $color = get_color($region['terrain']);
        $tag = get_image($region['terrain']);
        $x = transformx($region);
        $y = transformy($region);
        if (empty($bounds['xmin']) || $bounds['xmin'] > $x) $bounds['xmin'] = $x;
        if (empty($bounds['ymin']) || $bounds['ymin'] > $y) $bounds['ymin'] = $y;
        if (empty($bounds['xmax']) || $bounds['xmax'] < $x) $bounds['xmax'] = $x;
        if (empty($bounds['ymax']) || $bounds['ymax'] < $y) $bounds['ymax'] = $y;

        return sprintf($use_tag, $tag, $x, $y, $color);
    }
}

function warn($string) {
    echo "WARNING: $string\n";
}


$content = "";

$region = null;
while(!feof($myfile)) {
  $line = fgets($myfile);
  if (preg_match($preg_region, $line, $matches) == 1) {
      if ($region != null) {
          $x = $region['x'];
          $y = $region['y'];
          warn("REGION $x $y without terrain");
      }

      $region = parse_region($line, $matches);
  } else if (preg_match($preg_terrain, $line, $matches) == 1) {
    $region['terrain'] = $matches[1];
    $content .= output_region($region);
    $region = null;
  }
}

output_front();
fwrite($mysvg, $content);
output_back();

fclose($myfile);
fclose($mysvg);

?>

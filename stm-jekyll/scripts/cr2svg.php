<?php

# include dirname(__FILE__) . "/../vendor/erusev/parsedown/Parsedown.php";

function usage($argv) {
  echo "\n    Usage: $argv[0] [--html]|[--md input.md][--annotate] input.cr output.svg\n\n";
  exit(1);
}

$args=1;
if (empty($argv[$args]) || empty($argv[$args+1])) {
  usage($argv);
}

$htmlMode = false;
$mdMode = false;
$annotateMode = false;
if ($argv[$args] == '--html') {
  $htmlMode = true;
  $args++;
} else if ($argv[$args] == '--md') {
  $mdMode = true;
  $args++;
  $mdName = $argv[$args];
  if (empty($argv[$args]))
  usage($argv);
  $args += 1;
} else if ($argv[$args] == '--annotate') {
  $annotateMode = true;
  $args++;
}
if (empty($argv[$args]) || empty($argv[$args+1])) {
  usage($argv);
}

$inputName = $argv[$args];
$outputName = $argv[$args+1];

$crfile = fopen($inputName, "r") or die("Unable to open file $inputName!\n");
$svgfile = fopen($outputName, "w") or die("Unable to open file $outputName!\n");

$preg_region = '/^REGION (-{0,1}[0-9]+) (-{0,1}[0-9]+)( (-{0,1}[0-9]+)$){0,1}$/';
$preg_tagq = '/^"(.*)";(.*)$/';
$preg_tag = '/^(.*);(.*)$/';
$preg_block = '/^([A-Z]+) *(.*)$/';

function parse_region($line, $matches) {
  $parts = preg_split("/ /", $line);
  $region = array();
  $region['x'] = $matches[1];
  $region['y'] = $matches[2];
  if (!empty($matches[3]))
  $region['z'] = $matches[3];
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

$default_image = 'region';

$images = array(
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
    return null;
  else {
    return $images[$terrain];
  }
}

$html_template = <<<EOT
---
name: %1\$s
description: %2\$s
layout: crsvg
custom-javascript-list:
  - crstuff.js
---
<h1>%1\$s</h1>
<div id="tooltip" display="none" style="position: absolute; display: none;"></div>

<div id="svg" style = "max-width: 800px; max-height: 500px; line-height: 3em; overflow:scroll; border: thin #000 solid; padding: 5px;">
%3\$s
</div>
<div id="details">You need to enable Javascript for this to work.</div>
%4\$s
EOT;

$md_template = <<<EOT
---
%2\$s
list: true
layout: crsvg
custom-javascript-list:
  - crstuff.js
---
<h1>%1\$s</h1>
<div id="tooltip" display="none" style="position: absolute; display: none;"></div>

<div id="svg" style = "max-width: 800px; max-height: 500px; line-height: 3em; overflow:scroll; border: thin #000 solid; padding: 5px;">
%3\$s
</div>
<div id="details">You need to enable Javascript for this to work.</div>
%4\$s
EOT;

$annotation = <<<EOT
list: true
layout: crsvg
custom-javascript-list:
  - crstuff.js
EOT;

$annotation2 = <<<EOT
{% for cr in page.crs %}
  - [{{ cr }}]({{ cr | replace: ".cr", ".html" }})
{% endfor %}
EOT;


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

$use_tag = <<<EOT
<a href="#%1\$s" onmousemove="showTooltip(evt, '%2\$s');"
onmouseout="hideTooltip();"
onclick="showDescription(event, '%1\$s');">
<use xlink:href="#%3\$s" id="%1\$s"
x="%4\$d" y="%5\$d" %6\$s>
<title>%2\$s</title>
<desc>%7\$s</desc>
</use></a>

EOT;

$details_tag = <<<EOT
<h2>%s</h2><p><b>%s</b><br />%s</p>
EOT;



$rwidth = 100;
$yoff = $rwidth * .5;

function include_image($image) {
  $filename = "images/$image.svg";
  if (file_exists($filename)) {
    $contents = file_get_contents($filename);
    $contents = preg_replace('|^.*<svg.*(<image.*/>).*</svg>.*$|s', "\n".'$1'."\n", $contents);
    return "\n<g id='$image'>\n$contents\n</g>\n";
  } else {
    warn("$filename not found.");
    return "";
  }
}

function output_front() {
  global $front_matter, $bounds, $images;
  $content = sprintf($front_matter,
  ($bounds['xmax'] - $bounds['xmin'] + 110) / 2,
  ($bounds['ymax'] - $bounds['ymin'] + 110) / 2,
  $bounds['xmin'] - 5, $bounds['ymin'] -5,
  $bounds['xmax'] - $bounds['xmin'] + 110, $bounds['ymax'] - $bounds['ymin'] + 110);

  foreach ($images as $terrain => $image) {
    $content .= include_image($image);
  }

  return $content . "</defs>\n      <g>\n";
}

function output_back() {
  global $back_matter;
  return $back_matter;
}

function transformx($region) {
  global $rwidth, $yoff;
  return round($region['x'] * $rwidth + $region['y'] * $yoff);
}

function transformy($region) {
  global $rwidth;
  return round($region['y'] * - $rwidth * 3 / 4);
}

$display_tag = array ("x" => false, "y" => false, "z" => false, "Terrain" => false, "Name" => false);

function output_region($region) {
  global $default_image, $use_tag, $details_tag, $bounds, $display_tag;
  if ($region == null)
  return;

  if (empty($region['Terrain'])) {
    $x = $region['x'];
    $y = $region['y'];
    warn("REGION $x $y without terrain");
  }

  if (empty($region['z'])) {
    $color = get_color($region['Terrain']);
    $tag = get_image($region['Terrain']);
    if ($tag === null) {
      $tag = $default_image;
      $color = "fill=\"$color\"";
    } else {
      $color = "";
    }
    $xx = $region['x'];
    $yy = $region['y'];
    $x = transformx($region);
    $y = transformy($region);
    $tt = '';
    $desc = '';
    if (!empty($region['Name'])) {
      $tt = $region['Name'];
    } else {
      $tt = $region['Terrain'];
    }
    $tt .= " ($xx, $yy)";
    /*if (!empty($region['Name'])) */
    {
      $b = '';
      foreach ($region as $key => $value) {
        if (!isset($display_tag[$key]) || $display_tag[$key] === true) {
          $b .= "<i>" . $key . "</i> ". $value . "<br />";
        }
      }
      $desc = sprintf($details_tag, $tt, $region['Terrain'], $b);
    }
    $id = "r_";
    if ($xx < 0) $id .= "m" . -$xx; else $id .= $xx;
    if ($yy < 0) $id .= "_m" . -$yy; else $id .= "_" . $yy;

    if (empty($bounds['xmin']) || $bounds['xmin'] > $x) $bounds['xmin'] = $x;
    if (empty($bounds['ymin']) || $bounds['ymin'] > $y) $bounds['ymin'] = $y;
    if (empty($bounds['xmax']) || $bounds['xmax'] < $x) $bounds['xmax'] = $x;
    if (empty($bounds['ymax']) || $bounds['ymax'] < $y) $bounds['ymax'] = $y;

    return sprintf($use_tag, $id, $tt, $tag, $x, $y, $color, $desc);
  }
}

function warn($string) {
  echo "WARNING: $string\n";
}


$content = "";

if ($annotateMode) {
  $content = file_get_contents($inputName);
  $content = preg_replace('/^---$(.*)^---$(.*)/sm', "---$1\n$annotation\n---$2\n$annotation2", $content);
} else {
  $region = null;
  $block = null;
  while(!feof($crfile)) {
    $line = fgets($crfile);
    $tag = null;
    if (preg_match($preg_region, $line, $matches) == 1) {
      $content .= output_region($region);
      $block = "REGION";
      $region = parse_region($line, $matches);
    } else if (preg_match($preg_tagq, $line, $matches) == 1) {
      $value = $matches[1];
      $tag = $matches[2];
    } else if (preg_match($preg_tag, $line, $matches) == 1) {
      $value = $matches[1];
      $tag = $matches[2];
    } else if (preg_match($preg_block, $line, $matches) == 1){
      $block = $matches[1];
    }
    if (!empty($tag)) {
      if ($block == "REGION") {
        $region[$tag] = $value;
      }
    }
  }

  $content .= output_region($region);

  $content = output_front() . $content . output_back();
}

if ($htmlMode) {
  $details = "";
  $description = "";
  $title = basename($inputName);

  $content = sprintf($html_template, $title, $description, $content, $details);
}

if ($mdMode) {
  $details = "";
  $description = "";
  if (!empty($mdName)) {
    $details = file_get_contents($mdName);
    preg_match('/^---$(.*?(^name: *(.*?) *$).*?)^---$(.*)/sm', $details, $matches);
    $title = $matches[3];
    $head = $matches[1];
    $details = $matches[4];
  }
  if (empty($title)) {
    $title = basename($inputName);
  }

  $content = sprintf($md_template, $title, $head, $content, $details);
}

fwrite($svgfile, $content);

fclose($crfile);
fclose($svgfile);

?>

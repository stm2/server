<?php

if (empty($argv[1]) || empty($argv[2])) {
  echo "\n    Usage: $argv[0] [--html] input.cr output.svg\n\n";
  return;
}


$htmlMode = false;
if ($argv[1] == "--html") {
  $htmlMode = true;
  $inputName = $argv[2];
  $outputName = $argv[3];
} else {
  $inputName = $argv[1];
  $outputName = $argv[2];
}


$myfile = fopen($inputName, "r") or die("Unable to open file!");

$mysvg = fopen($outputName, "w");

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
<!DOCTYPE html>
<html>
<meta charset="UTF-8">
<script>
window.onload = function(){
  let element = document.getElementById("description");
  element.innerHTML ="Klicke auf eine Region f&uuml;r mehr Details.";
}
function showTooltip(evt, tooltip) {
  if (tooltip) {
    let element = document.getElementById("tooltip");
    element.innerHTML = tooltip;
    element.style.display = "block";
    element.style.left = evt.pageX + 10 + 'px';
    element.style.top = evt.pageY + 10 + 'px';
  }
}
function showDescription(e, id) {
  e.preventDefault();
  if (id) {
    let element = document.getElementById("description");
    let desc = document.getElementById(id).children[1].innerHTML;
    element.innerHTML = desc;
    element.style.display = "block";
  }
  return false;
}

function hideTooltip() {
  var element = document.getElementById("tooltip");
  element.style.display = "none";
}
</script>
<body>

<h1>Eressea</h1>
<div id="tooltip" display="none" style="position: absolute; display: none;"></div>

<div style = "width: 800px; height: 500px; line-height: 3em; overflow:scroll; border: thin #000 solid; padding: 5px;">
%s
</div>
<div id="description">You need to enable Javascript for this to work.</div>

</body>
</html>
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
<a href="xxx.html" onmousemove="showTooltip(evt, '%s');"
onmouseout="hideTooltip();"
onclick="showDescription(event, '%s');">
<use xlink:href="#%s" id="%s"
x="%d" y="%d" %s>
<title>%s</title>
<desc>%s</desc>
</use></a>

EOT;

$description_tag = <<<EOT
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
  global $default_image, $use_tag, $description_tag, $bounds, $display_tag;
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
      $desc = sprintf($description_tag, $tt, $region['Terrain'], $b);
    }
    $id = "r_";
    if ($xx < 0) $id .= "m" . -$xx; else $id .= $xx;
    if ($yy < 0) $id .= "_m" . -$yy; else $id .= "_" . $yy;

    if (empty($bounds['xmin']) || $bounds['xmin'] > $x) $bounds['xmin'] = $x;
    if (empty($bounds['ymin']) || $bounds['ymin'] > $y) $bounds['ymin'] = $y;
    if (empty($bounds['xmax']) || $bounds['xmax'] < $x) $bounds['xmax'] = $x;
    if (empty($bounds['ymax']) || $bounds['ymax'] < $y) $bounds['ymax'] = $y;

    return sprintf($use_tag, $tt, $id, $tag, $id, $x, $y, $color, $tt, $desc);
  }
}

function warn($string) {
  echo "WARNING: $string\n";
}


$content = "";

$region = null;
$block = null;
while(!feof($myfile)) {
  $line = fgets($myfile);
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

if ($htmlMode) {
  $content = sprintf($html_template, $content);
}

fwrite($mysvg, $content);

fclose($myfile);
fclose($mysvg);

?>

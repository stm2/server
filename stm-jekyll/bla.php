<html><body>
<?php

echo "<p>Hello</p>";
$svg_content = file_get_contents("example.svg");
echo '<div style = "width: 800px; height: 500px; line-height: 3em; overflow:scroll; border: thin #000 solid; padding: 5px;">';
echo preg_replace("|.*(<svg.*)|s", '$1', $svg_content);
echo '</div>';

// phpinfo();
?>
</body>
</html>

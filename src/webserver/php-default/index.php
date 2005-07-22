<?php

print "<html>";

$downloads = load_amule_vars("downloads");

print "<table>";
foreach ($downloads as $file) {
	print "<tr>";
	print "<td>";
	print $file->name;
	print "</td>";
	print "</tr>";
}
print "</table>";

print "</html>";

?>


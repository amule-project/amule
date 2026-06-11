<!doctype html>
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link href="style.css" rel="stylesheet" type="text/css">
</head>

<body class="frame">

<table class="w100p">
  <tr>
    <td class="w50p al-center va-bottom"> 
      <form name="formlink" method="post" action="footer.php">
        <input name="ed2klink" type="text" id="ed2klink" size="50">
        <select name="selectcat" id="selectcat">
        <?php
		$cats = amule_get_categories();
		if ( $HTTP_GET_VARS["Submit"] != "" ) {
			$link = $HTTP_GET_VARS["ed2klink"];
			$target_cat = $HTTP_GET_VARS["selectcat"];
			$target_cat_idx = 0;

			foreach($cats as $i => $c) {
				if ( $target_cat == $c) $target_cat_idx = $i;
			}

			if ( strlen($link) > 0 ) {
				$links = split("ed2k://", $link);
				foreach($links as $linkn) {
				    amule_do_ed2k_download_cmd("ed2k://" . $linkn, $target_cat_idx);
				}
			}
		}

		foreach($cats as $c) {
			echo  '<option>', htmlspecialchars($c), '</option>';
		}
	?>
        </select>
        <input type="submit" name="Submit" value="Download link">
      </form></td>
  </tr>
</table>
<?php
	if ( $HTTP_GET_VARS["Submit"] != "" ) {
		echo '<script type="text/javascript">window.top.location = "amuleweb-main-dload.php";</script>';
	}
?>
</body>
</html>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule-frame-bottom</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<style type="text/css">
<!--
body {
	background-color: #3399FF;
}
-->
</style></head>
<script language="JavaScript" type="text/JavaScript">

function refreshFrames()
{
	top.frames["mainFrame"].location = "amuleweb-main-dload.php"
	top.frames.mainFrame.location.reload();
}

</script>

<body>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <!--DWLayoutTable-->
  <tr>
    <td width="70" height="41">&nbsp;</td>
    <td width="100%" valign="top"><form name="formlink" method="post" action="footer.php">
        <input type="submit" name="Submit" value="Download link" onclick="refreshFrames()">
        <input name="ed2klink" type="text" id="ed2klink" size="100">
        <img src="arrow-r.png" width="42" height="23">
        <select name="selectcat" id="selectcat">
        <?php
				var_dump($HTTP_GET_VARS);
			$cats = amule_get_categories();
			if ( $HTTP_GET_VARS["Submit"] != "" ) {
				$link = $HTTP_GET_VARS["ed2klink"];
				$target_cat = $HTTP_GET_VARS["selectcat"];
				$target_cat_idx = 0;
            	foreach($cats as $i => $c) {
            		if ( $target_cat == $c) $target_cat_idx = $i;
            	}
            	if ( strlen($link) > 0 ) {
            		amule_do_ed2k_download_cmd($link, $target_cat_idx);
            	}
			}
			foreach($cats as $c) {
				echo  '<option>', $c, '</option>';
			}
        ?>
        </select>
        </form></td>
    <td width="29">&nbsp;</td>
  </tr>
  <tr>
    <td height="1"></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td></td>
  </tr>
</table>
</body>
</html>

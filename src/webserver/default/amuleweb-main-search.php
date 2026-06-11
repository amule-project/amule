<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>aMule control panel</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">

<script type="text/javascript" src="common.js"></script>
<link href="style.css" rel="stylesheet" type="text/css">
<script language="JavaScript" type="text/JavaScript">
function formCommandSubmit(command)
{
	<?php
		if ($_SESSION["guest_login"] != 0) {
				echo 'alert("You logged in as guest - commands are disabled");';
				echo "return;";
		}
	?>
	var frm=document.forms.mainform
	frm.command.value=command
	frm.submit()
}

</script>
</head>
<body background="images/fond.gif" leftmargin="0" topmargin="0" marginwidth="0" marginheight="0">
<table width="100%" height="100%" border="0" cellpadding="0" cellspacing="0">
  <tr valign="top"> 
    <td width="143" height="64"><img src="images/logo.png" width="143" height="64"></td>
    <td width="100%" height="64" align="right" background="images/fond_haut.png"> <table border="0" cellspacing="0" cellpadding="0">
        <tr> 
          <td><a class="navbutton nav-transfer" href="amuleweb-main-dload.php" title="Transfers"></a></td>
          <td><a class="navbutton nav-shared" href="amuleweb-main-shared.php" title="Shared files"></a></td>
          <td><a class="navbutton nav-search" href="amuleweb-main-search.php" title="Search"></a></td>
          <td><a class="navbutton nav-servers" href="amuleweb-main-servers.php" title="Servers"></a></td>
          <td><a class="navbutton nav-kad" href="amuleweb-main-kad.php" title="Kad"></a></td>
          <td><a class="navbutton nav-stats" href="amuleweb-main-stats.php" title="Statistics"></a></td>
          <td><img src="images/col.png"></td>
          <td width="10"></td>
          <td width="190" align="right" class="texteinv"><a href="login.php">exit</a><br> 
            <a href="amuleweb-main-log.php">log &bull;</a> <a href="amuleweb-main-prefs.php">configuration</a></td>
          <td width="10"></td>
        </tr>
      </table></td>
  </tr>
  <tr align="center" valign="top"> 
    <td colspan="2">
        <table width="100%" border="0" cellspacing="0" cellpadding="0">
          <caption>
        SEARCH
        </caption>
          <tr> 
            <td width="24"><img src="images/tab_top_left.png" width="24" height="24"></td>
            <td background="images/tab_top.png">&nbsp;</td>
            <td width="24"><img src="images/tab_top_right.png" width="24" height="24"></td>
          </tr>
          <tr> 
            <td width="24" background="images/tab_left.png">&nbsp;</td>
            
      <td bgcolor="#FFFFFF"><form name="mainform" action="amuleweb-main-search.php" method="post">
              <table width="100%" border="0" align="center" cellpadding="4" cellspacing="0">
                <tr align="center"> 
                  <td align="center">
<input type="hidden" name="command" value=""> 
                    <input name="searchval" type="text" id="searchval4" size="60"> 
                    <input name="Search" type="submit" id="Search4" value="Search" onClick="javascript:formCommandSubmit('search');"></td>
                  <td align="right">Availability :</td>
                  <td align="left"> 
                    <input name="avail" type="text" id="avail13" size="6"></td>
                  <td align="left">Min Size : </td>
                  <td align="left">
<input name="minsize" type="text" id="minsize2" size="5"> 
                    <select name="minsizeu" id="select8">
                      <option>Byte</option>
                      <option>KByte</option>
                      <option selected>MByte</option>
                      <option>GByte</option>
                    </select></td>
                </tr>
                <tr> 
                  <td align="center"><a href="amuleweb-main-search.php?search_sort=<?php
// Whitelist against the column keys my_cmp() actually understands
// (line 234-236 of this file). Anything else is dropped to empty,
// which falls through to the "no sort change" branch below. This
// avoids reflecting an attacker-controlled string into the rendered
// HTML (#869). Plain string-equality chain is the only matching
// primitive amuleweb's bundled PHP interpreter supports -- the
// classic `in_array()` / `htmlspecialchars()` builtins aren't
// registered (see src/webserver/src/php_core_lib.cpp), and array
// short syntax `[...]` doesn't parse either. The three whitelisted
// values are static alphanumeric column keys; no escaping needed.
$sort_raw = isset($HTTP_GET_VARS["sort"]) ? $HTTP_GET_VARS["sort"] : "";
if ($sort_raw == "size" || $sort_raw == "name" || $sort_raw == "sources") {
    echo($sort_raw);
}
?>">Click here to update the search results</a> </td>
                  <td align="right">Search type :</td>
                  <td> 
                    <select name="searchtype" id="select">
                      <option selected>Local</option>
                      <option>Global</option>
                      <option>Kad</option>
                    </select></td>
                  <td>Max Size : </td>
                  <td>
<input name="maxsize" type="text" id="maxsize4" size="5"> 
                    <select name="maxsizeu" id="select10">
                      <option>Byte</option>
                      <option>KByte</option>
                      <option selected>MByte</option>
                      <option>GByte</option>
                    </select></td>
                </tr>
              </table>
              <table width="100%"  border="0" align="center" cellpadding="0" cellspacing="0">
                <tr>
                  <th>&nbsp;</th>
                  <th><a href="amuleweb-main-search.php?sort=name">File Name</a></th>
                  <th><a href="amuleweb-main-search.php?sort=size">Size</a></th>
                  <th><a href="amuleweb-main-search.php?sort=sources">Sources</a></th>
    </tr><tr><td colspan="9" height="1" bgcolor="#000000"></td></tr>
    <?php
		function CastToXBytes($size)
		{
			if ( $size < 1024 ) {
				$result = $size . " b";
			} elseif ( $size < 1048576 ) {
				$result = ($size / 1024.0) . "kb";
			} elseif ( $size < 1073741824 ) {
				$result = ($size / 1048576.0) . "mb";
			} else {
				$result = ($size / 1073741824.0) . "gb";
			}
			return $result;
		}

		//
		// declare it here, before any function reffered it in "global"
		//
		$sort_order;$sort_reverse;

		function my_cmp($a, $b)
		{
			global $sort_order, $sort_reverse;
			
			switch ( $sort_order) {
				case "size": $result = $a->size > $b->size; break;
				case "name": $result = $a->name > $b->name; break;
				case "sources": $result = $a->sources > $b->sources; break;
			}

			if ( $sort_reverse ) {
				$result = !$result;
			}

			return $result;
		}

		function str2mult($str)
		{
			$result = 1;
			switch($str) {
				case "Byte":	$result = 1; break;
				case "KByte":	$result = 1024; break;		
				case "MByte":	$result = 1024*1024; break;
				case "GByte":	$result = 1024*1024*1024; break;
			}
			return $result;
		}

		function cat2idx($cat)
		{
                	$cats = amule_get_categories();
                	$result = 0;
                	foreach($cats as $i => $c) {
                		if ( $cat == $c) $result = $i;
                	}
            		return $result;
		}

		if ($_SESSION["guest_login"] == 0) {
			if ( $HTTP_GET_VARS["command"] == "search") {
				$search_type = -1;
				switch($HTTP_GET_VARS["searchtype"]) {
					case "Local": $search_type = 0; break;
					case "Global": $search_type = 1; break;
					case "Kad": $search_type = 2; break;
				}
				$min_size = $HTTP_GET_VARS["minsize"] == "" ? 0 : $HTTP_GET_VARS["minsize"];
				$max_size = $HTTP_GET_VARS["maxsize"] == "" ? 0 : $HTTP_GET_VARS["maxsize"];
	
				$min_size *= str2mult($HTTP_GET_VARS["minsizeu"]);
				$max_size *= str2mult($HTTP_GET_VARS["maxsizeu"]);
				
				amule_do_search_start_cmd($HTTP_GET_VARS["searchval"],
					//$HTTP_GET_VARS["ext"], $HTTP_GET_VARS["filetype"],
					"", "",
					$search_type, $HTTP_GET_VARS["avail"], $min_size, $max_size);
			} elseif ( $HTTP_GET_VARS["command"] == "download") {
				foreach ( $HTTP_GET_VARS as $name => $val) {
					// this is file checkboxes
					if ( (strlen($name) == 32) and ($val == "on") ) {
						$cat = $HTTP_GET_VARS["targetcat"];
						$cat_idx = cat2idx($cat);
						amule_do_search_download_cmd($name, $cat_idx);
					}
				}
			} else {
			}
		}		
		$search = amule_load_vars("searchresult");

		$sort_order = $HTTP_GET_VARS["sort"];

		if ( $sort_order == "" ) {
			$sort_order = $_SESSION["search_sort"];
		} else {
			if ( $_SESSION["search_sort_reverse"] == "" ) {
				$_SESSION["search_sort_reverse"] = 0;
			} else {
				$_SESSION["search_sort_reverse"] = !$_SESSION["search_sort_reverse"];
			}
		}

		$sort_reverse = $_SESSION["search_sort_reverse"];
		if ( $sort_order != "" ) {
			$_SESSION["search_sort"] = $sort_order;
			usort(&$search, "my_cmp");
		}

		foreach ($search as $file) {
			print "<tr>";

			echo "<td class='texte'>", '<input type="checkbox" name="', $file->hash, '" >', "</td>";

			echo "<td class='texte texte-full-name texte-full-name-search'>", $file->name, "</td>";
			
			echo "<td class='texte' align='center'>", CastToXBytes($file->size), "</td>";

			echo "<td class='texte' align='center'>", $file->sources, "</td>";

			print "</tr><tr><td colspan='9' height='1' bgcolor='#c0c0c0'></td></tr>";
		}

	  ?>
    <tr align="right"> 
      <td colspan="4" scope="col">
        <input name="Download" type="submit" id="Download6" value="Download" onClick="javascript:formCommandSubmit('download');" >
        <select name="targetcat" id="select32">
          <?php
                	$cats = amule_get_categories();
                	foreach($cats as $c) {
                		echo "<option>", $c, "</option>";
                	}
                ?>
        </select></td>
  </table>
</form></td>
            <td width="24" background="images/tab_right.png">&nbsp;</td>
          </tr>
          <tr> 
            <td width="24"><img src="images/tab_bottom_left.png" width="24" height="24"></td>
            <td background="images/tab_bottom.png">&nbsp;</td>
            <td width="24"><img src="images/tab_bottom_right.png" width="24" height="24"></td>
          </tr>
        </table></td>
  </tr>
  <tr valign="bottom"> 
    <td height="25" colspan="2"> <table width="100%" height="40" border="0" cellpadding="0" cellspacing="0">
        <tr align="center" valign="middle"> 
          <td width="50%"> <iframe name="stats" src="footer.php" height="35" width="100%" scrolling="no" frameborder="0">edklink</iframe> 
          </td>
          <td width="50%"> <iframe name="stats" src="stats.php" height="35" width="100%" scrolling="no" frameborder="0">connection</iframe> 
          </td>
        </tr>
      </table></td>
  </tr>
</table>
</body>
</html>

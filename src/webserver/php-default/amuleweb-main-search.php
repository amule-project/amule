<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule download page</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf8">
<style type="text/css">
<!--
body {
	margin-left: 0px;
	margin-top: 0px;
	margin-right: 0px;
	margin-bottom: 0px;
	background-color: #003399;
}
-->
</style>
</head>
<script language="JavaScript" type="text/JavaScript">
function formCommandSubmit(command)
{
	var frm=document.forms.mainform
	frm.command.value=command
	frm.submit()
}

</script>

<body>
<form name="mainform" action="amuleweb-main-search.php" method="post">
<table width="100%"  border="0" bgcolor="#0099CC">
  <tr>
    <td width="100%"><table width="100%"  border="0" cellpadding="2" cellspacing="2">
      <tr>
        <td colspan="5" scope="col"><table width="100%"  border="1" align="left" cellpadding="0" cellspacing="0">
          <tr>
            <td width="625" bgcolor="#0099CC">
              <input type="hidden" name="command" value="">
              <input name="searchval" type="text" id="searchval" size="75">
              <input name="Search" type="submit" id="Search" value="Search" onClick="javascript:formCommandSubmit('search');">
              <div align="left">
              </div></td>
            <td width="278" bgcolor="#0099CC">
            <input name="Download" type="submit" id="Download" value="Download" onClick="javascript:formCommandSubmit('download');" >
            <img src="arrow-r.png" width="42" height="23">              <select name="targetcat" id="targetcat">
                <?php
                	$cats = amule_get_categories();
                	foreach($cats as $c) {
                		echo "<option>", $c, "</option>";
                	}
                ?>
              </select></td>
          </tr>
          <tr>
            <td colspan="2" bgcolor="#0099CC"><div align="left">
              <label>Min size</label>
            &nbsp;
                <input name="minsize" type="text" id="minsize" size="5">
                <select name="minsizeu" id="minsizeu">
                    <option>Byte</option>
                    <option>KByte</option>
                    <option selected>MByte</option>
                    <option>GByte</option>
                </select>              
                <label>Max size</label>
&nbsp;
    <input name="maxsize" type="text" id="maxsize" size="5">
    <select name="maxsizeu" id="maxsizeu">
          <option>Byte</option>
          <option>KByte</option>
          <option selected>MByte</option>
          <option>GByte</option>
    </select>  
    <label>Availability</label>
    <input name="avail" type="text" id="avail" size="6">
    <label>Search type </label>
    <select name="searchtype" id="searchtype">
      <option selected>Local</option>
      <option>Global</option>
    </select>
            </div></td>
            </tr>
        </table></td>
        </tr>
      <tr>
        <th width="22" scope="col">&nbsp;</th>
        <th width="349" nowrap scope="col"><div align="left"><a href="amuleweb-main-search.php?sort=name" target="mainFrame">Filename</a></div></th>
        <th width="99" nowrap scope="col"><div align="left"><a href="amuleweb-main-search.php?sort=size" target="mainFrame">Size</a></div></th>
        <th width="96" nowrap scope="col"><div align="left"><a href="amuleweb-main-search.php?sort=sources" target="mainFrame">Sources</a></th>
        <th width="222" nowrap scope="col">&nbsp;</th>
        </tr>

	  <?php
		function CastToXBytes($size)
		{
			if ( $size < 1024 ) {
				$result = $size . " bytes";
			} elseif ( $size < 1048576 ) {
				$result = ($size / 1024.0) . "KB";
			} elseif ( $size < 1073741824 ) {
				$result = ($size / 1048576.0) . "MB";
			} elseif ( $size < 1099511627776 ) {
				$result = ($size / 1073741824.0) . "GB";
			} else {
				$result = "Too big";
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
				case "MByte":	$result = 1012*1024; break;
				case "GByte":	$result = 1012*1024*1024; break;
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
		
		if ( $HTTP_GET_VARS["command"] == "search") {
			$is_global = $HTTP_GET_VARS["searchtype"] == "Global" ? 1 : 0;

			$min_size = $HTTP_GET_VARS["minsize"] == "" ? 0 : $HTTP_GET_VARS["minsize"];
			$max_size = $HTTP_GET_VARS["maxsize"] == "" ? 0 : $HTTP_GET_VARS["maxsize"];

			$min_size *= str2mult($HTTP_GET_VARS["minsizeu"]);
			$max_size *= str2mult($HTTP_GET_VARS["maxsizeu"]);
			
			amule_do_search_start_cmd($HTTP_GET_VARS["searchval"],
				//$HTTP_GET_VARS["ext"], $HTTP_GET_VARS["filetype"],
				"", "Any",
				$is_global, $HTTP_GET_VARS["avail"], $min_size, $max_size);
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
			usort($search, "my_cmp");
		}

		foreach ($search as $file) {
			print "<tr>";

			echo "<td>", '<input type="checkbox" name="', $file->hash, '" >', "</td>";

			echo "<td nowrap>", $file->short_name, "</td>";
			
			echo "<td>", CastToXBytes($file->size), "</td>";

			echo "<td>", $file->sources, "</td>";

			print "</tr>";
		}

	  ?>

    </table></td>
  </tr>
</table>
</form>
</body>
</html>

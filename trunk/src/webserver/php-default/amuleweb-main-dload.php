<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>amule download page</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
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
</style></head>

<body>
<table width="100%"  border="1" bgcolor="#0099CC">
  <tr>
    <td height="20">&nbsp;</td>
  </tr>
  <tr>
    <td><table width="100%"  border="0">
      <tr>
        <th width="22" scope="col">&nbsp;</th>
        <th width="213" scope="col"><div align="left"><a href="amuleweb-main-dload.php?sort=name" target="mainFrame">Filename</a></div></th>
        <th width="329" scope="col"><div align="left">
          <div align="left">Progress</div></th>
        <th width="214" scope="col"><div align="left">
          <div align="left"><a href="amuleweb-main-dload.php?sort=size" target="mainFrame">Size</a></div></th>
        <th width="22" scope="col">&nbsp;</th>
        <th width="22" scope="col">&nbsp;</th>
        <th width="22" scope="col">&nbsp;</th>
        <th width="22" scope="col">&nbsp;</th>
        <th width="22" scope="col">&nbsp;</th>
        <th width="27" scope="col">&nbsp;</th>
      </tr>
      <tr>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
        <td scope="col">&nbsp;</td>
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

		// FIXME: replace with switch when supported by interpreter
		function StatusString($file)
		{
			if ( $status == 7 ) {
				return "Paused";
			}	elseif ( $file->src_count_xfer > 0 ) {
				return "Downloading";
			} else {
				return "Waiting";
			}
			return $file->status;
		}

		//
		// declare it here, before any function reffered it in "global"
		//
		$sort_order;$sort_reverse;

		function my_cmp($a, $b)
		{
			global $sort_order, $sort_reverse;
			if ( $sort_order == "size" ) {
				$result = $a->size > $b->size;
			} elseif ( $sort_order == "name" ) {
				$result = $a->name > $b->name;
			}

			if ( $sort_reverse ) {
				$result = !$result;
			}
			//var_dump($sort_reverse);
			return $result;
		}

		$downloads = load_amule_vars("downloads");

		$sort_order = $HTTP_GET_VARS["sort"];

		if ( $sort_order == "" ) {
			$sort_order = $_SESSION["download_sort"];
		} else {
			if ( $_SESSION["sort_reverse"] == "" ) {
				$_SESSION["sort_reverse"] = 0;
			} else {
				$_SESSION["sort_reverse"] = !$_SESSION["sort_reverse"];
			}
		}
		//var_dump($_SESSION);
		$sort_reverse = $_SESSION["sort_reverse"];
		if ( $sort_order != "" ) {
			$_SESSION["download_sort"] = $sort_order;
			usort($downloads, "my_cmp");
		}
		foreach ($downloads as $file) {
			print "<tr>";

			print "<td>";
			print "</td>";

			print "<td>";
			print $file->name;
			print "</td>";

			$progress_image = "dyn_". $file->hash . ".png";
			echo "<td> <img src=", $progress_image, "></td>";

			echo "<td>", CastToXBytes($file->size), "</td>";

			echo "<td>", StatusString($file), "</td>";

			echo "<td>", ($file->speed > 0) ? (CastToXBytes($file->speed) . "/s") : "-", "</td>";
			print "</tr>";
		}
	  ?>
    </table></td>
  </tr>
</table>
</body>
</html>

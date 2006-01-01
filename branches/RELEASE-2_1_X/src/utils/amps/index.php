<?php

/**
 * AMPS - AMule PHP Statistics
 * Written by überpenguin, AMPS is an adaptation of BigBob's aStats
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// If you want to mess with these settings, do so at your OWN RISK
// They are not all fully documented and it may not be obvious to anyone
// except myself what they do!

define(VERSION, "0.7.6");
define(AMULESIGDAT, "/tmp/amulesig.dat");
define(IMAGEPATH, "./images");
define(IMAGETYPE, "png");
define(CLEANSIGIMG, "baseimage.png");
define(RUNNINGIMG, "running.png");
define(RX0TX0IMG, "rx0tx0.png");
define(RX0TX1IMG, "rx0tx1.png");
define(RX1TX0IMG, "rx1tx0.png");
define(RX1TX1IMG, "rx1tx1.png");
define(HIGHIDIMGKADON, "conn_highid_kad_on.png");
define(HIGHIDIMGKADOFF, "conn_highid_kad_off.png");
define(HIGHIDIMGKADFW, "conn_highid_kad_fw.png");
define(LOWIDIMGKADFW, "conn_lowid_kad_fw.png");
define(LOWIDIMGKADOFF, "conn_lowid_kad_off.png");
define(LOWIDIMGKADON, "conn_lowid_kad_on.png");
define(NOCONNIMGKADON, "noconn_kad_on.png");
define(NOCONNIMGKADFW, "noconn_kad_fw.png");
define(NOCONNIMGKADOFF, "noconn_kad_off.png");
define(TIMEGENIMG, "clock.png");
define(SERVERIMG, "serverbox.png");
define(SHAREDFILESIMG, "files.png");
define(QUEUEIMG, "queue.png");
define(ERRFONTPADDING, 5);
define(FONTSIZE, 9);
define(FONTFILE, "./LucidaSansRegular.ttf");
define(IMAGETEXTCOLOR, "ffffff");
$completelangs = array("ca", "de", "en", "es", "eu", "fr", "fi", "hu", "it",
	"nl", "pl", "pt", "pt_BR");
// end constants

// initialize values array

$values = array("running"=>false, "psuptime"=>"", "connected"=>"0",
	"servername"=>"", "serverip"=>"0.0.0.0", "highlowid"=>"", "rxspeed"=>
	"0.0", "txspeed"=>"0.0", "queuedclients"=>"0", "sharedfiles"=>"0",
	"nick"=>"", "rxtotal"=>"0.0", "txtotal"=>"0.0", "muleversion"=>"0.0.0"
	);

// make sure gd and freetype are loaded; we need them for the signature image
if (!extension_loaded("gd"))
{
	echo "gd extension missing";
	exit();
}

if (!in_array("imagettftext", get_extension_funcs("gd")))
{
	echo "FreeType extensions missing from gd";
	exit();
}

// lang can come from two places; as an argument to the script (i.e.
// index.php?lang=xx or as an HTTP POST variable. The latter takes prescedence
// over the former.

if (isset($_GET["lang"]))
	$lang = $_GET["lang"];

if (isset($_POST["lang"]))
	$lang = $_POST["lang"];

if (isset($lang))
{
	$temp = @fopen("./langs/".$lang.".inc", "r");
	if (!$temp)
	{
		$lang = strtolower($lang);
		$temp = @fopen("./langs/".$lang.".inc", "r");
	}

	if (!$temp)
	{
		require("./langs/en.inc");
	}
	else
	{
		require("./langs/".$lang.".inc");
		fclose($temp);
	}
}
else
{
	$lang = "en";
	require("./langs/en.inc");
}

// if the variable $sig_image is set, only output the signature image

if (isset($_GET["sig_image"]))
{
	$ret = get_stats();
	if (!$ret)
		output_error_img($text["sigfileerr"]." (".AMULESIGDAT.
			")");
	else
		output_sig_image();

	exit();
}

/**
 * Retrieves the statistics, of course! First checks to see if the process is
 * actually running. If not, the function returns true without attempting to
 * read the aMule signature file. If the process IS running, the function reads
 * the appropriate data from the aMule signiture file, stores them in the
 * associative array $values and returns true on success, and false if there
 * is a failure reading the signature file.
 */

function get_stats()
{
	global $values, $text;

	// the ps command should output something like 1-05:23:45, in
	// days-hours:minutes:seconds format.

	// alternative ps command; doesn't work on *BSD!
	// $values["psuptime"] = trim(exec("ps --no-header -C amule -o etime"));

	$values["psuptime"] = trim(exec("ps ax -o etime,comm --no-header | ".
		"awk '/amule$/ {print $1}' | head -n 1"));

	if (!$values["psuptime"])
	{
		$values["running"] = false;
		return true;
	}
	else
	{
		$values["running"] = true;

		// Uncomment to test etime parser...
		// $values["psuptime"] = "3442-12:34:55";
		if (strlen($values["psuptime"]) >= 5)
			$uptimestr = substr($values["psuptime"], -5, 2).
				$text["minabbr"]." ".substr($values["psuptime"],
				-2).$text["secabbr"];
		if (strlen($values["psuptime"]) >= 8)
			$uptimestr = substr($values["psuptime"], -8, 2).
				$text["hourabbr"]." ".$uptimestr;
		if (strlen($values["psuptime"]) >= 10)
			$uptimestr = substr($values["psuptime"], 0, strlen(
				$values["psuptime"]) - 9).$text["dayabbr"]." ".
				$uptimestr;

		$values["psuptime"] = $uptimestr;

		$sigfile = @fopen(AMULESIGDAT, "r");

		if (!$sigfile)
			return false;
		else
		{
			$values["connected"] = trim(fgets($sigfile));
			$values["servername"] = trim(fgets($sigfile));
			$values["serverip"] = trim(fgets($sigfile));
			$values["serverport"] = trim(fgets($sigfile));
			$values["highlowid"] = trim(fgets($sigfile));
			$values["kad"] = trim(fgets($sigfile));
			$values["rxspeed"] = trim(fgets($sigfile));
			$values["txspeed"] = trim(fgets($sigfile));
			$values["queuedclients"] = trim(fgets($sigfile));
			$values["sharedfiles"] = trim(fgets($sigfile));
			$values["nick"] = trim(fgets($sigfile));
			$values["rxtotal"] = trim(fgets($sigfile));
			$values["txtotal"] = trim(fgets($sigfile));
			$values["muleversion"] = trim(fgets($sigfile));
			fclose($sigfile);

			return true;
		}
	}
}

/**
 * This function creates the signiture image and writes out in JPEG format (may
 * change this to PNG or add an option for image time in the future). The
 * function assumes get_stats() has already been called and will use whatever
 * values are in the $values array.
 */

function output_sig_image()
{
	global $values, $text;

	// open the base image for writing on. If unsuccessful, exit with an
	// error image.

	$finalimg = @imagecreatefrompng(IMAGEPATH."/".CLEANSIGIMG);

	if (!$finalimg)
	{
		output_error_img($text["baseimgerr"]);
		exit();
	}

	// open up all the icons

	$runningimg = @imagecreatefrompng(IMAGEPATH."/".RUNNINGIMG);
	$sharedfilesimg = @imagecreatefrompng(IMAGEPATH."/".SHAREDFILESIMG);
	$serverimg = @imagecreatefrompng(IMAGEPATH."/".SERVERIMG);
	$queueimg = @imagecreatefrompng(IMAGEPATH."/".QUEUEIMG);
	$timegenimg = @imagecreatefrompng(IMAGEPATH."/".TIMEGENIMG);
	
	if($values["kad"] == "2") {
		if ($values["highlowid"] == "H" && $values["connected"] == "1")
			$idimg = @imagecreatefrompng(IMAGEPATH."/".HIGHIDIMGKADON);
		else if ($values["highlowid"] == "L" && $values["connected"] == "1")
			$idimg = @imagecreatefrompng(IMAGEPATH."/".LOWIDIMGKADON);
		else
			$idimg = @imagecreatefrompng(IMAGEPATH."/".NOCONNIMGKADON);
	} else if ($values["kad"] == "1") {
		if ($values["highlowid"] == "H" && $values["connected"] == "1")
                        $idimg = @imagecreatefrompng(IMAGEPATH."/".HIGHIDIMGKADFW);
                else if ($values["highlowid"] == "L" && $values["connected"] == "1")
                        $idimg = @imagecreatefrompng(IMAGEPATH."/".LOWIDIMGKADFW);
                else
                        $idimg = @imagecreatefrompng(IMAGEPATH."/".NOCONNIMGKADFW);	
	} else {
		if ($values["highlowid"] == "H" && $values["connected"] == "1")
                        $idimg = @imagecreatefrompng(IMAGEPATH."/".HIGHIDIMGKADOFF);
                else if ($values["highlowid"] == "L" && $values["connected"] == "1")
                        $idimg = @imagecreatefrompng(IMAGEPATH."/".LOWIDIMGKADOFF);
                else
                        $idimg = @imagecreatefrompng(IMAGEPATH."/".NOCONNIMGKADOFF);
	}

	if (($values["rxspeed"] == "0.0" && $values["txspeed"] == "0.0") ||
		!$values["running"])
		$speedimg = @imagecreatefrompng(IMAGEPATH."/".RX0TX0IMG);
	else if ($values["rxspeed"] == "0.0" && $values["txspeed"] != "0.0")
		$speedimg = @imagecreatefrompng(IMAGEPATH."/".RX0TX1IMG);
	else if ($values["rxspeed"] != "0.0" && $values["txspeed"] == "0.0")
		$speedimg = @imagecreatefrompng(IMAGEPATH."/".RX1TX0IMG);
	else if ($values["rxspeed"] != "0.0" && $values["txspeed"] != "0.0")
		$speedimg = @imagecreatefrompng(IMAGEPATH."/".RX1TX1IMG);

	// check to make sure all the icons were successfully opened. If not,
	// output an image containing the appropriate error message and exit
	// the script here.

	if (! ($runningimg && $sharedfilesimg && $serverimg && $queueimg &&
		$timegenimg && $idimg && $speedimg))
	{
		output_error_img($text["iconerr"]);
		exit();
	}

	// place the icons onto the base image

	imagecopy($finalimg, $runningimg, 5, 5, 0, 0, imagesx($runningimg) - 1,
		imagesy($runningimg) - 1);
	imagecopy($finalimg, $sharedfilesimg, 249, 71, 0, 0,
		imagesy($sharedfilesimg) - 1, imagesy($sharedfilesimg) - 1);
	imagecopy($finalimg, $idimg, 5, 27, 0, 0, imagesx($idimg) - 1,
		imagesy($idimg) - 1);
	imagecopy($finalimg, $serverimg, 5, 49, 0, 0, imagesx($serverimg) - 1,
		imagesy($serverimg) - 1);
	imagecopy($finalimg, $speedimg, 5, 71, 0, 0, imagesx($speedimg) - 1,
		imagesy($speedimg) - 1);
	imagecopy($finalimg, $queueimg, 249, 93, 0, 0, imagesx($queueimg) - 1,
		imagesy($queueimg) - 1);
	imagecopy($finalimg, $timegenimg, 5, 93, 0, 0, imagesx($timegenimg) -
		1, imagesy($timegenimg) - 1);

	// allocate white for the text color

	sscanf(IMAGETEXTCOLOR, "%2x%2x%2x", $red, $green, $blue);
	$fgcolor = imagecolorallocate($finalimg, $red, $green, $blue);

	// aMule version and process status

	if ($values["running"])
		imagettftext($finalimg, FONTSIZE, 0, 26, 19, $fgcolor,
			FONTFILE, "aMule ".$values["muleversion"].
			" ".$text["runtimemsg"]." ".$values["psuptime"]);
	else
		imagettftext($finalimg, FONTSIZE, 0, 26, 19, $fgcolor,
			FONTFILE, "aMule ".$text["norunmsg"]);

	// connection status and nickname

	if($values["kad"] == "2")
	{
		if ($values["running"] && $values["connected"] == "2")
			imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
				"aMule ".$text["connmsg"]." ".$text["kadonmsg"]);
		else if ($values["running"] && $values["connected"] != "0" &&
			$values["highlowid"] == "H")
			imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
				$values["nick"]." ".$text["highidmsg"]." ".$text["kadonmsg"]);
		else if ($values["running"] && $values["connected"] != "0" &&
			$values["highlowid"] != "H")
			imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
				$values["nick"]." ".$text["lowidmsg"]." ".$text["kadonmsg"]);
		else if ($values["running"] && $values["connected"] == "0")
			imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
				$values["nick"]." ".$text["offrunmsg"]." ".
				$text["amulenorun"]." ".$text["kadonmsg"]);
		else
			imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
				$text["offline"]." ".$text["amulenorun"]);
	} else if($values["kad"] == "1")
        {
                if ($values["running"] && $values["connected"] == "2")
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                "aMule ".$text["connmsg"]." ".$text["kadfwmsg"]);
                else if ($values["running"] && $values["connected"] != "0" &&
                        $values["highlowid"] == "H")
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                $values["nick"]." ".$text["highidmsg"]." ".$text["kadfwmsg"]);
                else if ($values["running"] && $values["connected"] != "0" &&
                        $values["highlowid"] != "H")
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                $values["nick"]." ".$text["lowidmsg"]." ".$text["kadfwmsg"]);
                else if ($values["running"] && $values["connected"] == "0")
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                $values["nick"]." ".$text["offrunmsg"]." ".
                                $text["amulenorun"]." ".$text["kadfwmsg"]);
                else
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                $text["offline"]." ".$text["amulenorun"]);
        } else {
                if ($values["running"] && $values["connected"] == "2")
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                "aMule ".$text["connmsg"]." ".$text["kadoffmsg"]);
                else if ($values["running"] && $values["connected"] != "0" &&
                        $values["highlowid"] == "H")
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                $values["nick"]." ".$text["highidmsg"]." ".$text["kadoffmsg"]);
                else if ($values["running"] && $values["connected"] != "0" &&
                        $values["highlowid"] != "H")
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                $values["nick"]." ".$text["lowidmsg"]." ".$text["kadoffmsg"]);
                else if ($values["running"] && $values["connected"] == "0")
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                $values["nick"]." ".$text["offrunmsg"]." ".
                                $text["amulenorun"]." ".$text["kadoffmsg"]);
                else
                        imagettftext($finalimg, FONTSIZE, 0, 25, 41, $fgcolor, FONTFILE,
                                $text["offline"]." ".$text["amulenorun"]);
        }



	// shared files

	if ($values["running"] && $values["connected"] == "1")
		imagettftext($finalimg, FONTSIZE, 0, 270, 85, $fgcolor,
			FONTFILE, $text["sharedfiles"].": ".
			$values["sharedfiles"]);
	else
		imagettftext($finalimg, FONTSIZE, 0, 270, 85, $fgcolor,
			FONTFILE, $text["sharedfiles"].": ".$text["na"]);

	// server name, ip, port

	if ($values["running"] && $values["connected"] == "1")
		imagettftext($finalimg, FONTSIZE, 0, 25, 63, $fgcolor, FONTFILE,
			$values["servername"]." (".$values["serverip"].":".
			$values["serverport"].")");
	else
		imagettftext($finalimg, FONTSIZE, 0, 25, 63, $fgcolor, FONTFILE,
			$text["na"]);

	// RX & TX

	if ($values["running"])
		imagettftext($finalimg, FONTSIZE, 0, 25, 85, $fgcolor, FONTFILE,
			$text["rx"].": ".$values["rxspeed"].$text["transrate"].
			" | ".$text["tx"].": ".$values["txspeed"].
			$text["transrate"]);
	else
		imagettftext($finalimg, FONTSIZE, 0, 25, 85, $fgcolor, FONTFILE,
			$text["rx"].": ".$text["na"]." | ".$text["tx"].": ".
			$text["na"]);

	// queued clients

	if ($values["running"])
		imagettftext($finalimg, FONTSIZE, 0, 270, 107, $fgcolor,
			FONTFILE, $text["queuedclients"].": ".
			$values["queuedclients"]);
	else
		imagettftext($finalimg, FONTSIZE, 0, 270, 107, $fgcolor,
			FONTFILE, $text["queuedclients"].": ".$text["na"]);

	// RFC 2822 Datestamp

	imagettftext($finalimg, FONTSIZE, 0, 25, 107, $fgcolor, FONTFILE,
		date("r"));

	// outputs the signature image based on the value of IMAGETYPE. The case
	// for png and default are the same; in other words, PNG is the default
	// image output type. Note that your installation of GD has to support
	// the type of image you are outputting here.

	switch (IMAGETYPE)
	{
		case "jpg":
		case "jpeg":
		{
			header("Content-type: image/jpeg");
			imagejpeg($finalimg, "", 100);
		}
		case "gif":
		{
			header("Content-type: image/gif");
			imagegif($finalimg);
		}
		case "png":
		default:
		{
			header("Content-type: image/png");
			imagepng($finalimg);
		}
	}
}

/**
 * This function outputs a simple image containing an error message specified
 * by the only parameter, $err_message. The image is black text and border on
 * a white background.
 */

function output_error_img($err_message)
{
	// determine the size of the text string and add to it the amount of
	// padding specified earlier.

	$fontdims = imagettfbbox(FONTSIZE, 0, FONTFILE, $err_message);

	$min_x = min($fontdims[0], $fontdims[2], $fontdims[4], $fontdims[6]);
	$max_x = max($fontdims[0], $fontdims[2], $fontdims[4], $fontdims[6]);
	$min_y = min($fontdims[1], $fontdims[3], $fontdims[5], $fontdims[7]);
	$max_y = max($fontdims[1], $fontdims[3], $fontdims[5], $fontdims[7]);

	$width = ($max_x - $min_x) + (2 * ERRFONTPADDING);
	$height = ($max_y - $min_y) + (2 * ERRFONTPADDING);

	$img = @imagecreate($width, $height);

	$bgcolor = imagecolorallocate($img, 255, 255, 255);
	$fgcolor = imagecolorallocate($img, 0, 0, 0);
	imagesetthickness($img, 2);

	// draw the border

	imagerectangle($img, 1, 1, $width - 2, $height - 2, $fgcolor);

	// print the text

	imagettftext($img, FONTSIZE, 0, ERRFONTPADDING,	ERRFONTPADDING + 0.5 *
		$height, $fgcolor, FONTFILE, $err_message);

	header("Content-type: image/jpeg");
	imagejpeg($img, "", 100);
}

/**
 * Writes the non-dynamic leading part of the HTML that makes up the statistics
 * page. Right now this is just a stub for future theme support.
 */

function write_header()
{

?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">

<html>
<head>
	<title>AMPS - AMule PHP Statistics</title>
	<link href="./style.css" rel="stylesheet" type="text/css">
</head>
<body class="thebody">

<p align="center">

<table class="toplevel">
<tr>
<td>

<?php

}

/**
 * Writes the non-dynamic trailing part of the HTML that makes up the statistics
 * page. Right now this is just a stub for future theme support.
 */

function write_footer()
{

?>

</td>
</tr>
</table>

<p align="center">

<i>Generated by AMPS version <?php echo VERSION; ?></i><br>
<i><a href="http://www.amule-project.net/">http://www.amule-project.net/</a></i><br>
<i>&lt;uberpenguin at hotpop dot com&gt;</i>

</body>
</html>
<?php

}

/**
 * Write out the stats to the browser. I will probably change this later on
 * to be themable, but for now I lack the motivation...
 */

write_header();

$ret = get_stats();
if (!$ret)
	echo $text["sigfileerr"];
else
{
	// html strings that are used over and over

	$nastring = "\t\t\t\t\t<td class=\"organize\">".$text["na"].
		"</td>\n\t\t\t\t</tr>\n";
	$titlecolstart = "\t\t\t\t<tr>\n\t\t\t\t\t<td class=\"organize\">";
	$titlecolend = "</td>\n";
	$datacolstart = "\t\t\t\t\t<td class=\"organize\">";
	$datacolend = "</td>\n\t\t\t\t</tr>\n";

	$sectiontablestart = "\t<table class=\"sec\">\n\t\t".
		"<tr class=\"secheader\">\n\t\t\t<td class=\"secheader\">";
	$sectiontablenext = "</td>\n\t\t</tr>\n";
	$sectionend = "\t\t\t</table>\n\t\t\t</td>\n\t\t</tr>\n\t</table>".
		"\n\t<p>\n";
	$innertable = "\t\t<tr class=\"secbody\">\n\t\t\t".
		"<td class=\"secbody\">\n";
	$organizetable = "\t\t\t<table class=\"organize\">\n";

	// title

	echo "\t<table class=\"sec\">\n\t\t<tr class=\"secheader\">\n\t\t\t".
		"<td class=\"secheader\"><center><h2>AMPS - ".
		"AMule PHP Statistics</h2></center></td>\n\t\t</tr>\n\t".
		"</table>\n\t<p>\n";

	// first section

	echo $sectiontablestart.$text["general"].$sectiontablenext;

	echo $innertable;
	echo $organizetable;

	// client info

	echo $titlecolstart.$text["client"].$titlecolend;

	if ($values["running"])
		echo $datacolstart."aMule ".$values["muleversion"]." ".
			$text["runtimemsg"]." ".$values["psuptime"].$datacolend;
	else
		echo $datacolstart."aMule ".$text["norunmsg"].$datacolend;

	// ed2kstatus

	echo $titlecolstart.$text["ed2kstatus"].$titlecolend;

	if ($values["connected"] == "2")
		echo $datacolstart.$text["connecting"];
	else if ($values["connected"] == "1")
		echo $datacolstart.$text["online"];
	else
		echo $datacolstart.$text["offline"];

	if (!$values["running"])
		echo " ".$text["amulenorun"].$datacolend;
	else
		echo $datacolend;

	// kadstatus

        echo $titlecolstart.$text["kadstatus"].$titlecolend;

        if ($values["kad"] == "2")
                echo $datacolstart.$text["kadon"];
        else if ($values["kad"] == "1")
                echo $datacolstart.$text["kadfw"];
        else
                echo $datacolstart.$text["kadoff"];

        if (!$values["running"])
                echo " ".$text["amulenorun"].$datacolend;
        else
                echo $datacolend;

	// nick

	echo $titlecolstart.$text["nick"].$titlecolend;

	if ($values["running"])
		echo $datacolstart.$values["nick"].$datacolend;
	else
		echo $nastring;

	// RFC 2282 datestamp

	echo $titlecolstart.$text["localtime"].$titlecolend;
	echo $datacolstart.date("r").$datacolend;

	// RX speed

	echo $titlecolstart.$text["rxspeed"].$titlecolend;

	if ($values["running"])
		echo $datacolstart.$values["rxspeed"].$text["transrate"].
			$datacolend;
	else
		echo $nastring;

	// TX speed

	echo $titlecolstart.$text["txspeed"].$titlecolend;

	if ($values["running"])
		echo $datacolstart.$values["txspeed"].$text["transrate"].
			$datacolend;
	else
		echo $nastring;

	// queued clients

	echo $titlecolstart.$text["queuedclients"].$titlecolend;

	if ($values["running"])
		echo $datacolstart.$values["queuedclients"].$datacolend;
	else
		echo $nastring;

	// TX total

	echo $titlecolstart.$text["txtotal"].$titlecolend;

	if ($values["running"])
		echo $datacolstart.round($values["txtotal"] / 1073741824, 2).
			$text["gigabytes"].$datacolend;
	else
		echo $nastring;

	// RX total

	echo $titlecolstart.$text["rxtotal"].$titlecolend;

	if ($values["running"])
		echo $datacolstart.round($values["rxtotal"] / 1073741824, 2).
			$text["gigabytes"].$datacolend;
	else
		echo $nastring;

	// shared files

	echo $titlecolstart.$text["sharedfiles"].$titlecolend;

	if ($values["running"] && $values["connected"] == "1")
		echo $datacolstart.$values["sharedfiles"].$datacolend;
	else
		echo $nastring;

	// OS identification

	echo $titlecolstart.$text["osversion"].$titlecolend;
	echo $datacolstart.exec("uname -sr").$datacolend;

	// uptime

	echo $titlecolstart.$text["hostuptime"].$titlecolend;
	echo $datacolstart.exec("uptime").$datacolend;

	// break into next section

	echo $sectionend;
	echo $sectiontablestart.$text["server"].$sectiontablenext;
	echo $innertable;
	echo $organizetable;

	// server name

	echo $titlecolstart.$text["servername"].$titlecolend;

	if ($values["running"] && $values["connected"] == "1")
		echo $datacolstart.$values["servername"].$datacolend;
	else
		echo $nastring;

	// server IP:port

	echo $titlecolstart.$text["serveraddr"].$titlecolend;

	if ($values["running"] && $values["connected"] == "1")
		echo $datacolstart.$values["serverip"].":".
			$values["serverport"].$datacolend;
	else
		echo $nastring;

	// ED2K link

	echo $titlecolstart.$text["ed2klink"].$titlecolend;

	if ($values["running"] && $values["connected"] == "1")
		echo $datacolstart."<a href=\"ed2k://".$values["servername"].
			"|".$values["serverip"]."|".$values["serverport"].
			"|/\">ed2k://".$values["servername"]."|".
			$values["serverip"]."|".$values["serverport"].
			"|/</a>".$datacolend;
	else
		echo $nastring;

	// break into next section

	echo $sectionend;
	echo $sectiontablestart.$text["signature"].$sectiontablenext;
	echo $innertable;
	echo $organizetable;

	// signature image

	global $lang;

	echo "\t\t\t\t<tr><td><a href=\"".$_SERVER['PHP_SELF']."?lang=".$lang."&sig_image\"><img border=\"0\"".
		"src=\"".$_SERVER['PHP_SELF']."?lang=".$lang."&sig_image\" alt=\"\"></a></td></tr>\n";

	// language selection

        echo $sectionend;
	echo $sectiontablestart;

	echo "\n\t\t\t\t<form method=\"POST\" action=\"".$_SERVER['PHP_SELF']."\">\n\t\t\t\t\t".
		$text["language"].":&nbsp;&nbsp;<select name=\"lang\">\n";
	foreach ($completelangs as $langcode)
		if ($langcode == $lang)
			echo "\t\t\t\t\t\t<option value=\"".$langcode."\" selected>".$langcode.
				"</option>\n";
		else
			echo "\t\t\t\t\t\t<option value=\"".$langcode."\">".$langcode."</option>\n";
	echo "\t\t\t\t\t</select>\n\t\t\t\t\t&nbsp;&nbsp;<input type=\"submit\" value=\"".
		$text["submit"]."\">\n\t\t\t\t</form>\n";
	echo "\t\t\t</td>\n\t\t</tr>\n\t</table>";
}

write_footer();

?>

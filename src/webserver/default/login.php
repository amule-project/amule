<!doctype html>
<html>
<head>
<title>aMule control panel</title><script language="JavaScript" type="text/javascript">

function login_init()
{
	document.login.pass.focus();
}

</script>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link href="style.css" rel="stylesheet" type="text/css">
</head>

<body class="main" onload="login_init();">
<table class="w100p h180">
  <tr>
    <td class="al-center va-middle">
      <table class="w70p h90p center-table bg-black spacing1">
        <tr>
          <td><table class="w100p h100p bg-white">
              <tr class="va-top">
               <th class="w366 h180"><img src="images/loginlogo.jpg" width="366" height="180"></th>
                <th class="w100p login-banner">
                  <form action="" method="post" name="login">
                    Enter password : 
                    <input name="pass" size="20" value="" type="password">
                    &nbsp; 
                    <input name="submit" type="submit" value="Submit">
                    &nbsp;&nbsp;
<?php
	if ( $_SESSION["login_error"] != "" ) {
		echo "<br><span class=\"login-error\">", $_SESSION["login_error"], "</span>&nbsp;&nbsp;";
	}
?>
                    </form></th>
              </tr>
            </table></td>
        </tr>
      </table>
    </td>
  </tr>
</table>
</body>
</html>

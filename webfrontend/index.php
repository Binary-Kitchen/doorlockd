<!DOCTYPE html>

<?php
	function tellLock( $pAction, $pUser, $pPass, $pToken, $pIp ){

		$pAuthenticated = true;

		$json = '{
			"user":' . json_encode( $pUser ) . ',
			"password":' . json_encode( $pPass ) . ',
			"action":' . json_encode( $pAction ) . ',
			"token":' . json_encode( $pToken ) . ',
			"ip":' . json_encode( $pIp ) . ',
			"authenticate":' . json_encode( $pAuthenticated ) . '
		}'."\n";

		$address = "127.0.0.1";
		$port = "5555";

		$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
		if ($socket === false) {
			echo "socket_create() failed: " . socket_strerror(socket_last_error()) . "\n";
		}

		$result = socket_connect($socket, $address, $port);
		if ($result === false) {
			echo "socket_connect() failed: ($result) " . socket_strerror(socket_last_error($socket)) . "\n";
		}

		socket_write($socket, $json, strlen($json));
		
		$result = socket_read($socket, 1024);

		socket_close($socket);

		return $result;
	}

	function err2str( $code ) {
		switch ( $code ) {
			case 0:
				return "Success";
				break;
			case 1:
				return "Fail";
				break;
			case 2:
				return "Already Unlocked"; // Authentication successful, but door is already unlocked
				break;
			case 3:
				return "Already Locked"; // Authentication successful, but door is already locked
				break;
			case 4:
				return "NotJson"; // Request is not a valid JSON object
				break;
			case 5:
				return "Json Error"; // Request is valid JSON, but does not contain necessary material
				break;
			case 6:
				return "Invalid Token"; // Request contains invalid token
				break;
			case 7:
				return "Invalid Credentials"; // Invalid LDAP credentials
				break;
			case 8:
				return "Invalid IP";
				break;
			case 9:
				return "Unknown Action"; // Unknown action
				break;
			case 10:
				return "LDAP Init error"; // Ldap initialization failed
				break;
			default:
				return "Unknown error";
				break;
		}
	}

	$showLoginForm = false;
	$showSuccess = false;
	$showFailure = false;

	$pIp = $_SERVER[ 'REMOTE_ADDR' ];

	if( $_SERVER[ 'REQUEST_METHOD' ] == "POST" ) {

		$pUser = $_POST[ 'user' ];
		$pPass = $_POST[ 'pass' ];
		$pToken = $_POST[ 'token' ];
		$pAction = $_POST[ 'action' ];

		$lSuccess = tellLock( $pAction, $pUser, $pPass, $pToken, $pIp );

		if ($lSuccess == 0) {
			$showSuccess = true;
		} else {
			http_response_code( 401 );
			$failureMsg = err2str($lSuccess);
			$showFailure = true;
		}
	} else {
		// This is done by apache mod_rewrite
		$pToken = $_GET[ 'token' ];
		$lToken = preg_replace( '/[^0-9a-f]/i', "", $pToken );
		if(strlen($lToken) != 16) {
			http_response_code( 404 );
			$showFailure = true;
		}
		$showLoginForm = true;
	}
?>

<html>

<head>
<title>Login</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
</head>

<body>

	<style>
		* {
			font: normal 30px Arial,sans-serif;
		}
		body {
			background-color: #037;
			color: white;
			background-image: url('logo.svg' );
			background-repeat: repeat;
			background-size: 300%;
			background-position: -200px -100px;
		}
		form {
			position: relative;
			display: block;
			width: auto;
			text-align: center;
		}
		input {
			position: relative;
			display: block;
			width: auto;
			width: 100%;
		}
		button {
			width: 100%;
			margin-top: 40px;
		}
	</style>

	<?php if( $showLoginForm ): ?>

		<form name="login" method="post" action="/">
			<label for="user">User</label>
			<input id="user" type="text" name="user">

			<label for="pass">Pass</label>
			<input id="pass" type="password" name="pass">

			<input type="hidden" name="token" value="<?php echo $lToken;?>">

			<button name="action" value="unlock">Open</button>
			<hr/>
			<button name="action" value="lock">Lock</button>
		</form>

	<?php elseif( $showSuccess ): ?>

		<h1>Welcome Cpt. Cook</h1>

	<?php elseif( $showFailure ): ?>

		<h1>Something went wrong: <?php echo $failureMsg; ?></h1>

	<?php endif; ?>

</body>

</html>

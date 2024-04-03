<?php

if(isset($_GET["user"])) {
   $user = $_GET["user"]; 
   $doxi = $_GET["bpm"]; 
   $dbpm = $_GET["spo2"]; 
   $dtemp = $_GET["temp"]; 
   
   $servername = "localhost";
   $username = "khuclukz_vwuser";
   $password = "J-:zjfNx,dJcH6R";
   $database_name = "khuclukz_vitalwatch";
   


	$tanggal = date("Y-m-d", time() + (86400*$ddate));
	
//	echo $tanggal;
   
   // Create MySQL connection fom PHP to MySQL server
   $connection = new mysqli($servername, $username, $password, $database_name);
   // Check connection
   if ($connection->connect_error) {
      die("MySQL connection failed: " . $connection->connect_error);
   }

	//$sql = "INSERT INTO daily_data (date,time,location,prediction,status) VALUES (now(),now(),$room,$status)";
	$sql = "INSERT INTO daily_data (id_user,date,time,data_oximeter,data_bpm,data_temp) VALUES ($user,CURDATE(),CURRENT_TIME(),$doxi,$dbpm,$dtemp)";
echo $sql;
   if ($connection->query($sql) === TRUE) {
      echo "New record created successfully";
   } else {
      echo "Error: " . $sql . " => " . $connection->error;
   }

   $connection->close();
} else {
   echo "a is not set in the HTTP request";
}
?>
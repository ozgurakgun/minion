#! /bin/bash

flags=$1

cmake -D SUPPORTSGACELEMENT=OFF -D SUPPORTSGACELEMENTLONG=OFF -D SUPPORTSGACLEX=OFF -D SUPPORTSGACLEXLONG=OFF -D SUPPORTSGACSQUAREPACK=OFF -D SUPPORTSGACSQUAREPACKLONG=OFF -D SUPPORTSGACLIST=OFF -D SUPPORTSGACLISTNOCOPY=OFF -D SUPPORTSGACNDLIST=OFF .  
cmake -D SUPPORTSGACELEMENT=ON . 
make $flags  minion 

cmake -D SUPPORTSGACELEMENT=OFF -D SUPPORTSGACELEMENTLONG=OFF -D SUPPORTSGACLEX=OFF -D SUPPORTSGACLEXLONG=OFF -D SUPPORTSGACSQUAREPACK=OFF -D SUPPORTSGACSQUAREPACKLONG=OFF -D SUPPORTSGACLIST=OFF -D SUPPORTSGACLISTNOCOPY=OFF -D SUPPORTSGACNDLIST=OFF .  

cmake -D SUPPORTSGACELEMENTLONG=ON . 
make $flags  minion 

cmake -D SUPPORTSGACELEMENT=OFF -D SUPPORTSGACELEMENTLONG=OFF -D SUPPORTSGACLEX=OFF -D SUPPORTSGACLEXLONG=OFF -D SUPPORTSGACSQUAREPACK=OFF -D SUPPORTSGACSQUAREPACKLONG=OFF -D SUPPORTSGACLIST=OFF -D SUPPORTSGACLISTNOCOPY=OFF -D SUPPORTSGACNDLIST=OFF .  
  
cmake -D SUPPORTSGACLEX=ON . 
make $flags  minion 

cmake -D SUPPORTSGACELEMENT=OFF -D SUPPORTSGACELEMENTLONG=OFF -D SUPPORTSGACLEX=OFF -D SUPPORTSGACLEXLONG=OFF -D SUPPORTSGACSQUAREPACK=OFF -D SUPPORTSGACSQUAREPACKLONG=OFF -D SUPPORTSGACLIST=OFF -D SUPPORTSGACLISTNOCOPY=OFF -D SUPPORTSGACNDLIST=OFF .  
  
cmake -D SUPPORTSGACLEXLONG=ON . 
make $flags  minion 

cmake -D SUPPORTSGACELEMENT=OFF -D SUPPORTSGACELEMENTLONG=OFF -D SUPPORTSGACLEX=OFF -D SUPPORTSGACLEXLONG=OFF -D SUPPORTSGACSQUAREPACK=OFF -D SUPPORTSGACSQUAREPACKLONG=OFF -D SUPPORTSGACLIST=OFF -D SUPPORTSGACLISTNOCOPY=OFF -D SUPPORTSGACNDLIST=OFF .  
  
cmake -D SUPPORTSGACSQUAREPACK=ON . 
make $flags  minion 

cmake -D SUPPORTSGACELEMENT=OFF -D SUPPORTSGACELEMENTLONG=OFF -D SUPPORTSGACLEX=OFF -D SUPPORTSGACLEXLONG=OFF -D SUPPORTSGACSQUAREPACK=OFF -D SUPPORTSGACSQUAREPACKLONG=OFF -D SUPPORTSGACLIST=OFF -D SUPPORTSGACLISTNOCOPY=OFF -D SUPPORTSGACNDLIST=OFF .  
  
cmake -D SUPPORTSGACSQUAREPACKLONG=ON . 
make $flags  minion 

cmake -D SUPPORTSGACELEMENT=OFF -D SUPPORTSGACELEMENTLONG=OFF -D SUPPORTSGACLEX=OFF -D SUPPORTSGACLEXLONG=OFF -D SUPPORTSGACSQUAREPACK=OFF -D SUPPORTSGACSQUAREPACKLONG=OFF -D SUPPORTSGACLIST=OFF -D SUPPORTSGACLISTNOCOPY=OFF -D SUPPORTSGACNDLIST=OFF .  
  
cmake -D SUPPORTSGACLIST=ON . 
make $flags  minion 

cmake -D SUPPORTSGACELEMENT=OFF -D SUPPORTSGACELEMENTLONG=OFF -D SUPPORTSGACLEX=OFF -D SUPPORTSGACLEXLONG=OFF -D SUPPORTSGACSQUAREPACK=OFF -D SUPPORTSGACSQUAREPACKLONG=OFF -D SUPPORTSGACLIST=OFF -D SUPPORTSGACLISTNOCOPY=OFF -D SUPPORTSGACNDLIST=OFF .  
  
cmake -D SUPPORTSGACNDLIST=ON . 
make $flags  minion 

cmake -D SUPPORTSGACELEMENT=OFF -D SUPPORTSGACELEMENTLONG=OFF -D SUPPORTSGACLEX=OFF -D SUPPORTSGACLEXLONG=OFF -D SUPPORTSGACSQUAREPACK=OFF -D SUPPORTSGACSQUAREPACKLONG=OFF -D SUPPORTSGACLIST=OFF -D SUPPORTSGACLISTNOCOPY=OFF -D SUPPORTSGACNDLIST=OFF .  
  
cmake -D SUPPORTSGACLISTNOCOPY=ON . 
make $flags  minion 


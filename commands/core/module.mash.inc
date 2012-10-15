<?php


function module_mash_command(){

  $items = array();

   $items['create-module'] = array(
     'bootstrap' => MASH_BOOTSTRAP_MAGENTO_FULL,
     'description' => 'Create a new Magento module.',
     'arguments' => array(
       'name' => 'The name of the created module.',
     ),
     'callback' => 'mash_module_command_create',
     'aliases' => array('cm'),
   );


   return $items;


}

function mash_module_command_create(){

  mash_print('Create module');

}
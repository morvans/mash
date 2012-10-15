<?php


function module_mash_command(){

  $items = array();

  $items['create-module'] = array(
    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_FULL,
    'description' => 'Create a blank module structure.',
    'arguments' => array(
      'name' => 'The name of the new module.',
    ),
    'engines' => array('magento'),
    'callback' => 'mash_module_create_module',
    'aliases' => array('mc'),
  );


   return $items;


}

function mash_module_create_module($name = NULL){

  if(!isset($name) || empty($name)){
    $name = mash_prompt('Module name', NULL, TRUE);

  }

  mash_include_engine('magento', 'module');


  $module = new Module();
  $module->createModule($name);

}


<?php


function route_mash_command(){

  $items = array();

  $items['create-route'] = array(
    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_FULL,
    'description' => 'Create a blank module structure.',
    'arguments' => array(
      'name' => 'The name of the new module.',
    ),
    'engines' => array('magento'),
    'callback' => 'mash_route_create_route',
    'aliases' => array('rc'),
  );


   return $items;


}

function mash_route_create_route($module = NULL, $area = NULL, $frontName = NULL){

  if(!isset($module) || empty($module)){
    $module = mash_prompt('Module name', NULL, TRUE);
  }

  if(!isset($area) || empty($area)){
    $area = mash_prompt('Area', 'frontend', TRUE);
  }

  if(!isset($frontName) || empty($frontName)){
    $frontName = mash_prompt('Front name', strtolower($module), TRUE);
  }

  mash_include_engine('magento', 'module');
  mash_include_engine('magento', 'route');

  $route = new Route();
  $route->createRoute($module, $area, $frontName);

}


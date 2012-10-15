<?php

function cache_mash_command() {

  $items = array();

  $items['cache-clear'] = array(
    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_FULL,
    'description' => 'Clear a specific cache, or all Magento caches.',
    'arguments' => array(
      'type' => 'The particular cache to clear. Omit this argument to choose from available caches.',
    ),
    'callback' => 'mash_cache_command_clear',
    'aliases' => array('cc'),
  );


  return $items;

}

function mash_cache_command_clear() {

  drush_cache_clear_drush();

  date_default_timezone_set("Europe/Madrid");
  mash_print('Start Cleaning all caches in ' . mash_get_context('MASH_MAGENTO_ROOT') . ' at ... ' . date("Y-m-d H:i:s"));
  ini_set("display_errors", 1);



  $types = Mage::app()->getCacheInstance()->getTypes();

  try {
    mash_print('Cleaning data cache...');
    foreach ($types as $type => $data) {
      mash_print_prompt('Removing '.$type.' ... ');
      mash_print(Mage::app()->getCacheInstance()->clean($data["tags"]) ? "[OK]" : "[ERROR]");;
    }
  } catch (exception $e) {
    die("[ERROR:" . $e->getMessage() . "]");
  }

  echo "\n";

  try {
    mash_print_prompt( "Cleaning stored cache... ");
    mash_print( Mage::app()->getCacheInstance()->clean() ? "[OK]" : "[ERROR]");
  } catch (exception $e) {
    die("[ERROR:" . $e->getMessage() . "]");
  }

  try {
    mash_print_prompt( "Cleaning merged JS/CSS...");
    Mage::getModel('core/design_package')->cleanMergedJsCss();
    Mage::dispatchEvent('clean_media_cache_after');
    mash_print( "[OK]");
  } catch (Exception $e) {
    die("[ERROR:" . $e->getMessage() . "]");
  }

  try {
    mash_print_prompt( "Cleaning image cache... ");
    mash_print_prompt( Mage::getModel('catalog/product_image')->clearCache());
    mash_print( "[OK]");
  } catch (exception $e) {
    die("[ERROR:" . $e->getMessage() . "]");
  }

}

/**
 * Clear caches internal to drush core.
 */
function drush_cache_clear_drush() {
  mash_cache_clear_all(NULL, 'default'); // commandfiles, etc.
  mash_cache_clear_all(NULL, 'complete'); // completion
}
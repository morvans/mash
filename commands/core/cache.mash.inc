<?php

/**
 * Implementation of hook_mash_command().
 */
function cache_mash_command() {
  $items = array();

  // We specify command callbacks here because the defaults would collide with
  // the mash cache api functions.
//  $items['cache-get'] = array(
//    'description' => 'Fetch a cached object and display it.',
//    'examples' => array(
//      'mash cache-get schema' => 'Display the data for the cache id "schema" from the "cache" bin.',
//      'mash cache-get update_available_releases update' => 'Display the data for the cache id "update_available_releases" from the "update" bin.',
//    ),
//    'arguments' => array(
//      'cid' => 'The id of the object to fetch.',
//      'bin' => 'Optional. The cache bin to fetch from.',
//    ),
//    'required-arguments' => 1,
//    'options' => array(
//      'format' => 'Format to output the object. Use "print_r" for print_r (default), "export" for var_export, and "json" for JSON.',
//    ),
//    'callback' => 'mash_cache_command_get',
//    'aliases' => array('cg'),
//  );
  $items['cache-clear'] = array(
    'bootstrap' => MASH_BOOTSTRAP_MAX,
    'description' => 'Clear a specific cache, or all magento caches.',
    'arguments' => array(
      'type' => 'The particular cache to clear. Omit this argument to choose from available caches.',
    ),
    'callback' => 'mash_cache_command_clear',
    'aliases' => array('cc'),
  );
//  $items['cache-set'] = array(
//    'description' => 'Cache an object expressed in JSON or var_export() format.',
//    'arguments' => array(
//      'cid' => 'The id of the object to set.',
//      'data' => 'The object to set in the cache. Use \'-\' to read the object from STDIN.',
//      'bin' => 'Optional. The cache bin to store the object in.',
//      'expire' => 'Optional. CACHE_PERMANENT, CACHE_TEMPORARY, or a Unix timestamp.',
//    ),
//    'required-arguments' => 2,
//    'options' => array(
//      'format' => 'Format to parse the object. Use "string" for string (default), and "json" for JSON.',
//      'cache-get' => 'If the object is the result a previous fetch from the cache, only store the value in the "data" property of the object in the cache.',
//    ),
//    'callback' => 'mash_cache_command_set',
//    'aliases' => array('cs'),
//  );

  return $items;
}

/**
 * Command argument complete callback.
 *
 * @return
 *   Array of clear types.
 */
function cache_cache_command_clear_complete() {
  return array('values' => array_keys(mash_cache_clear_types()));
}

/**
 * Command callback for mash cache-clear.
 */
function mash_cache_command_clear($type = NULL) {
  $types = mash_cache_clear_types();

  // Check if the provided type ($type) is a valid cache type.
  if ($type && !array_key_exists($type, $types)) {
    return mash_set_error(dt("'!type' cache is not a valid cache type", array('!type' => $type)));
  }

  if ($type) {
    mash_op($types[$type]);
    if ($type == 'all' && !mash_has_boostrapped(MASH_BOOTSTRAP_MAGENTO_FULL)) {
      $type = 'mash';
    }
  } else {
    // Don't offer 'all' unless Mash has bootstrapped the Magento site
    if (!mash_has_boostrapped(MASH_BOOTSTRAP_MAGENTO_FULL)) {
      unset($types['all']);
    }
    $type = mash_choice($types, 'Enter a number to choose which cache to clear.', '!key');
    if ($type !== FALSE) {
      call_user_func($types[$type]);
    }
  }
  if ($type !== FALSE) {
    $site_label = '';
    if ($type != 'mash') {
      $self_name = mash_sitealias_bootstrapped_site_name();
      if (isset($self_name)) {
        $site_label = dt(' in !name', array('!name' => $self_name));
      }
    }
    mash_log(dt("'!name' cache was cleared!insitename", array('!name' => $type, '!insitename' => $site_label)), 'success');
  }
}

/**
 * Print an object returned from the cache.
 *
 * @param $cid
 *   The cache ID of the object to fetch.
 * @param $bin
 *   Optional parameter to specify a specific bin to fetch from.
 */
//function mash_cache_command_get($cid = NULL, $bin = NULL) {
//  if (!$cid) {
//    mash_log(dt('You must specify a cache id to fetch.'), 'error');
//    return;
//  }
//
//  if (!$bin) {
//    $bin = 'cache';
//  }
//
//  $result = cache_get($cid, $bin);
//  if (!empty($result)) {
//    mash_print(mash_format($result));
//  }
//  else {
//    mash_log(dt('The !cid object in the !bin cache bin was not found.', array('!cid' => $cid, '!bin' => $bin)), 'error');
//  }
//}

/**
 * Set an object in the cache.
 *
 * @param $cid
 *   The cache ID of the object to fetch.
 * @param $data
 *   The data to save to the cache, or '-' to read from STDIN.
 * @param $bin
 *   Optional parameter to specify a specific bin to fetch from.
 * @param $expire
 *   Optional parameter to specify the expiry of the cached object.
 */
//function mash_cache_command_set($cid = NULL, $data = '', $bin = NULL, $expire = CACHE_PERMANENT) {
//  if (!$bin) {
//    $bin = 'cache';
//  }
//
//  if ($data == '-') {
//    $data = stream_get_contents(STDIN);
//  }
//
//  // Now, we parse the object.
//  switch (mash_get_option('format', 'string')) {
//    case 'json':
//      $data = mash_json_decode($data);
//      break;
//  }
//
//  if (mash_get_option('cache-get')) {
//    $data = $data->data;
//  }
//
//  cache_set($cid, $data, $bin, $expire);
//}

function mash_cache_clear_types() {
  $types = array(
    'all' => 'mash_cache_clear_both',
    'mash' => 'mash_cache_clear_mash',
  );
  if (mash_has_boostrapped(MASH_BOOTSTRAP_MAGENTO_FULL)) {

    $types += array(
      'data' => 'mash_cache_clear_data_cache',
      'stored' => 'mash_cache_clear_stored_cache',
      'css-js' => 'mash_cache_clear_merged_css_js_cache',
      'image' => 'mash_cache_clear_image_cache',
    );
  }

  // Include the appropriate environment engine, so callbacks can use core
  // version specific cache clearing functions directly.
  //mash_include_engine('magento', 'environment');

  // Command files may customize $types as desired.
  mash_command_invoke_all_ref('mash_cache_clear', $types);

  return $types;
}


/**
 * Clear caches internal to mash core.
 */
function mash_cache_clear_mash() {
  mash_cache_clear_all(NULL, 'default'); // commandfiles, etc.
  mash_cache_clear_all(NULL, 'complete'); // completion
}

/**
 * Clear caches internal to Mash core and Magento.
 */
function mash_cache_clear_both() {
  mash_cache_clear_mash();
  if (mash_has_boostrapped(MASH_BOOTSTRAP_MAGENTO_FULL)) {
    mash_cache_flush_all_magento_caches();
  }
}

function mash_cache_flush_all_magento_caches() {

  mash_cache_clear_data_cache();
  mash_cache_clear_stored_cache();
  mash_cache_clear_merged_css_js_cache();
  mash_cache_clear_image_cache();

}

function mash_cache_clear_data_cache() {

  $types = Mage::app()->getCacheInstance()->getTypes();

  try {
    mash_log('Cleaning data cache...');
    foreach ($types as $type => $data) {
      mash_log('Removing ' . $type . ' ... ');
      if (!Mage::app()->getCacheInstance()->clean($data["tags"])) {
        return mash_set_error('MAGENTO_CACHE_CLEAR_ERROR', 'Cleaning data cache ' . $type["tags"] . ' failed');
      }
    }
    mash_log('Cleaned data cache...');
  } catch (exception $e) {
    return mash_set_error('MAGENTO_CACHE_CLEAR_ERROR', $e->getMessage());
  }

}

function mash_cache_clear_stored_cache() {
  try {
    mash_log("Cleaning stored cache... ");
    if (!Mage::app()->getCacheInstance()->clean()) {
      return mash_set_error('MAGENTO_CACHE_CLEAR_ERROR', 'Cleaning stored cache failed');
    }
  } catch (exception $e) {
    return mash_set_error('MAGENTO_CACHE_CLEAR_ERROR', $e->getMessage());
  }
}

function mash_cache_clear_merged_css_js_cache() {
  try {
    mash_log("Cleaning merged JS/CSS...");
    Mage::getModel('core/design_package')->cleanMergedJsCss();
    Mage::dispatchEvent('clean_media_cache_after');
    mash_log("Cleaned merged JS/CSS...");
  } catch (Exception $e) {
    return mash_set_error('MAGENTO_CACHE_CLEAR_ERROR', $e->getMessage());
  }
}

function mash_cache_clear_image_cache() {


  try {
    mash_log("Cleaning image cache... ");
    Mage::getModel('catalog/product_image')->clearCache();
    mash_log("Cleaned image cache... ");
  } catch (exception $e) {
    return mash_set_error('MAGENTO_CACHE_CLEAR_ERROR', $e->getMessage());
  }

}


<?php

/**
 * Implementation of hook_mash_command().
 */
function config_mash_command() {
  $items = array();

  $items['config-get-data'] = array(
    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_FULL,
    'description' => 'Get value of a config path.',
    'options' => array(
    ),
    'arguments' => array(
      'path' => 'Config data path',
      'scope' => 'Config data scope',
      'scope_id' => 'Config data scope_id',
    ),
    'callback' => 'mash_config_get_data',
    'aliases' => array('cget'),
  );

  $items['config-set-data'] = array(
    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_FULL,
    'description' => 'Set value of a config path.',
    'options' => array(
    ),
    'arguments' => array(
      'path' => 'Config data path',
      'value' => 'Config data value',
      'scope' => 'Config data scope',
      'scope_id' => 'Config data scope_id',
    ),
    'callback' => 'mash_config_set_data',
    'aliases' => array('cset'),
  );

  $items['config-remove-data'] = array(
    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_FULL,
    'description' => 'Set value of a config path.',
    'options' => array(
    ),
    'arguments' => array(
      'path' => 'Config data path',
      'scope' => 'Config data scope',
      'scope_id' => 'Config data scope_id',
    ),
    'callback' => 'mash_config_remove_data',
    'aliases' => array('cdel'),
  );

  $items['config-apply-data'] = array(
    'bootstrap' => MASH_BOOTSTRAP_MAGENTO_FULL,
    'description' => 'Set value of a config path.',
    'options' => array(
    ),
    'arguments' => array(
      'ini-file' => 'Path to .ini file containing configuraiton to apply',
    ),
    'callback' => 'mash_config_apply_data',
    'aliases' => array('capp'),
  );


  return $items;
}

function mash_config_set_data($path, $value, $scope = 'default', $scopeId = 0) {

  mash_log('mash_config_set_data()');

  mash_log('save config data for ' . $path);

  Mage::getConfig()->saveConfig($path, $value, $scope, $scopeId);
  Mage::getConfig()->removeCache();

}

function mash_config_get_data($path, $scope = 'default', $scopeId = 0) {

  mash_log('mash_config_get_data()');

  mash_log('get config data for ' . $path);

  $config = Mage::getConfig()->getNode($path, $scope, $scopeId);
  if( !$config ){
    mash_log('No configuration data for ' . $path . '('.$scope.':'.$scopeId.')', 'error');
    return FALSE;
  }

  mash_print($config);

}

function mash_config_remove_data($path, $scope = 'default', $scopeId = 0) {

  mash_log('mash_config_set_data()');

  mash_log('remove config data for ' . $path);

  Mage::getConfig()->deleteConfig($path, $scope, $scopeId);
  Mage::getConfig()->removeCache();

}

function mash_config_apply_data($ini_path){

  $config_ini = parse_ini_file($ini_path, TRUE);


  $rows[] = array('scope','scope_id', 'path', 'value');

  foreach($config_ini as $section => $config){

    if( preg_match('/^(default|(websites|stores):([0-9]+))$/', $section, $matches)){

      if( $matches[0] == 'default' ){
        $scope = 'default';
        $scope_id = (string) '0';
      }else{
        $scope = $matches[2];
        $scope_id = $matches[3];
      }

      foreach($config as $path => $value){

        $rows[] = array(
          'scope' => $scope,
          'scope_id' => $scope_id,
          'path' => $path,
          'value' => $value
        );

      }

    }else{

      mash_log('unknown section ' . $section, 'warning');

    }

  }

  mash_print_table($rows);
  array_shift($rows);

  foreach($rows as $row){
    mash_config_set_data($row['path'], $row['value'], $row['scope'], $row['scope_id']);
  }


}
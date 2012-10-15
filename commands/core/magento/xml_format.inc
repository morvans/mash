<?php


/**
 * Looks for the package path under local.
 *
 * @param $package - the package path name
 * @return the path to the specified package directory under app/code/local or
 *   FALSE if could not be found.
 */
function getPackagePath($package) {
  $path = FALSE;

  $localPath = getPath('local');
  if ($localPath) {
    $path = getPath($package, $localPath);
  }

  return $path;
}

/**
 * Looks for the module path under package.
 *
 * @param $package - the package path name
 * @param $module - the module path name
 * @return the path to the specified module directory under app/code/local/<package>
 *   or FALSE if could not be found.
 */
function getModulePath($package, $module) {

  $localPath = mash_get_context('MASH_SELECTED_MAGENTO_ROOT').'/app/code/local/';


  return $localPath.'/'.$package.'/'.$module;

}

/**
 * Looks for the <package>/<module>/etc/config.xml file.
 *
 * @param $package - the package path name
 * @param $module - the module path name
 * @return the path to and including the config.xml file or FALSE if could not
 *   be found.
 */
function getConfigPath($package, $module) {
  $path = FALSE;

  $modulePath = getModulePath($package, $module);
  if ($modulePath) {
    $configPath = $modulePath . '/etc/config.xml';
    if (file_exists($configPath)) {
      $path = $configPath;
    }
  }

  return $path;
}



function formatXml($xml) {
  // Loads the XSLT
  $xslDoc = new DOMDocument();
  $xslDoc->load(__DIR__ . "/format.xsl");

  // Loads the XML
  $xmlDoc = new DOMDocument();
  $xmlDoc->loadXML($xml->asXML());

  // Processes the XML
  $proc = new XSLTProcessor();
  $proc->importStylesheet($xslDoc);

  return $proc->transformToXML($xmlDoc);
}


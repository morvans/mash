<?php


class Module
{
  // Standard module config XML template for Magento
  const BASE_XML = '<?xml version="1.0" encoding="UTF-8"?><config><modules></modules></config>';

  protected $package;
  protected $module;
  protected $lowercaseModule;
  protected $packageModule;

  /**
  * Creates a module in Magento.
  *
  * @param $module - The fully qualified module name. Example: Package_Module. 
  *   If not specified, the user is prompted for it.
  */
  public function createModule($module) {
    // Request missing parameters
    if (!isset($module) || empty($module)) {
      throw new Exception('no name given');
//      $module = in('What is the fully qualified module name? Example: Package_Module.');
    }

    // Validates parameters
    $components = explode('_', $module);
    if (count($components) < 2) {
      mash_set_error('CREATE_MODULE_ERROR','Module name needs to be fully qualified. Example: Package_Module');
      return;
    }

    $this->package         = $components[0];
    $this->module          = $components[1];
    $this->lowercaseModule = strtolower($this->module);
    $this->packageModule   = $this->package . '_' . $this->module;

    // XML for the app/etc/modules/<Package>_<Module>.xml
    $this->createModulesXml();

    // XML for app/code/local/<Package>/<Module>/etc/config.xml
    $this->createModuleXml();

    // Any time XML has changed, need to clear cache
    mash_cache_flush_all_magento_caches();
  }

  /**
   * @return TRUE if the module exist. Checks for config.xml.
   */
  public function moduleExists() {
    return getConfigPath($this->package, $this->module);
  }

  /**
  * Creates the XML file in app/etc/modules/
  */
  protected function createModulesXml() {
    // Creates XML
    $packageModule = $this->packageModule;
    $xml = new SimpleXMLElement(Module::BASE_XML);
    $xml->modules->addChild($this->packageModule);
    $xml->modules->$packageModule->addChild('active', 'true');
    $xml->modules->$packageModule->addChild('codePool', 'local');

    // Formats XML with proper indention
    $formattedXml = formatXml($xml);

    // Writes XML to app/etc/modules/<Package>_<Module>.xml
    $rootPath = getRootPath();

    $rootPath = mash_get_context('MASH_SELECTED_MAGENTO_ROOT');

    $etcPath = $rootPath . 'app/etc/modules/';
    $fileHandler = fopen($etcPath . $this->packageModule . '.xml', 'w');
    fwrite($fileHandler, $formattedXml);
    fclose($fileHandler);
  }

  /*
  * XML for app/code/local/<Package>/<Module>/etc/config.xml
  */
  protected function createModuleXml() {
    // Creates XML
    $packageModule = $this->packageModule;
    $xml = new SimpleXMLElement(Module::BASE_XML);
    $xml->modules->addChild($packageModule);
    $xml->modules->$packageModule->addChild('version', '0.1.0');

    // Formats XML with proper indention
    $formattedXml = formatXml($xml);

    // Creates package and module directory if not already created
    $localPath = mash_get_context('MASH_SELECTED_MAGENTO_ROOT').'/app/code/local/';
    $modulePath = $localPath . $this->package . '/' . $this->module . '/etc/';
    mash_mkdir($modulePath);

    // Writes XML to app/code/local/<Package>/<Module>/etc/config.xml
    $fileHandler = fopen($modulePath . 'config.xml', 'w');
    fwrite($fileHandler, $formattedXml);
    fclose($fileHandler);
  }
}





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
    $configPath = $modulePath . 'etc/config.xml';
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


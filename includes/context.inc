<?php
/**
 * @file
 * The Mash context API implementation.
 *
 * This API acts as a storage mechanism for all options, arguments and
 * configuration settings that are loaded into mash.
 *
 * This API also acts as an IPC mechanism between the different mash commands,
 * and provides protection from accidentally overriding settings that are
 * needed by other parts of the system.
 *
 * It also avoids the necessity to pass references through the command chain
 * and allows the scripts to keep track of whether any settings have changed
 * since the previous execution.
 *
 * This API defines several contexts that are used by default.
 *
 * Argument contexts :
 *   These contexts are used by Mash to store information on the command.
 *   They have their own access functions in the forms of
 *   mash_set_arguments(), mash_get_arguments(), mash_set_command(),
 *   mash_get_command().
 *
 *     command : The mash command being executed.
 *     arguments : Any additional arguments that were specified.
 *
 * Setting contexts :
 *   These contexts store options that have been passed to the mash.php
 *   script, either through the use of any of the config files, directly from
 *   the command line through --option='value' or through a JSON encoded string
 *   passed through the STDIN pipe.
 *
 *   These contexts are accessible through the mash_get_option() and
 *   mash_set_option() functions.  See mash_context_names() for a description
 *   of all of the contexts.
 *
 *   Mash commands may also choose to save settings for a specific context to
 *   the matching configuration file through the mash_save_config() function.
 */


/**
 * Return a list of the valid mash context names.
 *
 * These context names are carefully ordered from
 * highest to lowest priority.
 *
 * These contexts are evaluated in a certain order, and the highest priority value
 * is returned by default from mash_get_option. This allows scripts to check whether
 * an option was different before the current execution.
 *
 *   Specified by the script itself :
 *     process  : Generated in the current process.
 *     cli      : Passed as --option=value to the command line.
 *     stdin    : Passed as a JSON encoded string through stdin.
 *     specific : Defined in a command-specific option record, and
 *                set in the command context whenever that command is used.
 *     alias    : Defined in an alias record, and set in the
 *                alias context whenever that alias is used.
 *
 *   Specified by config files :
 *     custom   : Loaded from the config file specified by --config or -c
 *     site     : Loaded from the mashrc.php file in the Magento site directory.
 *     magento   : Loaded from the mashrc.php file in the Magento root directory.
 *     user     : Loaded from the mashrc.php file in the user's home directory.
 *     home.mash Loaded from the mashrc.php file in the $HOME/.mash directory.
 *     system   : Loaded from the mashrc.php file in the system's $PREFIX/etc/mash directory.
 *     mash    : Loaded from the mashrc.php file in the same directory as mash.php.
 *
 *   Specified by the script, but has the lowest priority :
 *     default  : The script might provide some sensible defaults during init.
 */
function mash_context_names() {
  static $contexts = array(
    'process', 'cli', 'stdin', 'specific', 'alias',
    'custom', 'site', 'magento', 'user', 'home.mash', 'system',
    'mash', 'default');

  return $contexts;
}

/**
 * Return a list of possible mashrc file locations.
 *
 * @context
 *   A valid mash context from mash_context_names().
 * @prefix
 *   Optional. Specify a prefix to prepend to ".mashrc.php" when looking
 *   for config files. Most likely used by contrib commands.
 * @return
 *   An associative array containing possible config files to load
 *   The keys are the 'context' of the files, the values are the file
 *   system locations.
 */
function _mash_config_file($context, $prefix = NULL, $version = '') {
  $configs = array();
  $base_name = 'mash' . $version . 'rc.php';
  $config_file = $prefix ? $prefix . '.' . $base_name : $base_name;

  // Did the user explicitly specify a config file?
  if ($config_list = (array)mash_get_context('MASH_CONFIG')) {
    foreach ($config_list as $config) {
      if (is_dir($config)) {
        $config = $config . '/' . $config_file;
      }
      $configs['custom'][] = $config;
    }
  }

  if ($magento_root = mash_get_context('MASH_SELECTED_MAGENTO_ROOT')) {
    $configs['magento'] = $magento_root . '/sites/all/mash/' . $config_file;

    if ($site_path = $magento_root . '/' . mash_get_context('MASH_SELECTED_MAGENTO_SITE_CONF_PATH')) {
      $configs['site'] = $site_path . "/" . $config_file;
    }
  }

  // in the user home directory
  if (!is_null(mash_server_home())) {
    $configs['user'] = mash_server_home() . '/.' . $config_file;
  }

  // in $HOME/.mash directory
  $per_user_config_dir = mash_get_context('MASH_PER_USER_CONFIGURATION');
  if (!empty($per_user_config_dir)) {
    $configs['home.mash'] = $per_user_config_dir . '/' . $config_file;
  }

  // In the system wide configuration folder.
  $configs['system'] = mash_get_context('MASH_SITE_WIDE_CONFIGURATION') . '/' . $config_file;

  // in the mash installation folder
  $configs['mash'] = dirname(__FILE__) . '/../' . $config_file;

  return empty($configs[$context]) ? '' : $configs[$context];
}


/**
 * Load mashrc files (if available) from several possible locations.
 */
function mash_load_config($context) {
  mash_load_config_file($context, _mash_config_file($context));
  mash_load_config_file($context, _mash_config_file($context, '', MASH_MAJOR_VERSION));
}

function mash_load_config_file($context, $config_list) {
  foreach ((array)$config_list as $config) {
    if (file_exists($config)) {
      $options = $aliases = $command_specific = $override = array();
      mash_log(dt('Loading mashrc "!config" into "!context" scope.', array('!config' => realpath($config), '!context' => $context)), 'bootstrap');
      $ret = @include_once($config);
      if ($ret === FALSE) {
        mash_log(dt('Cannot open mashrc "!config", ignoring.', array('!config' => realpath($config))), 'warning');
        return FALSE;
      }
      if (!empty($options) || !empty($aliases) || !empty($command_specific) || !empty($override)) {
        $options = array_merge(mash_get_context($context), $options);
        $options['config-file'] = realpath($config);

        //$options['site-aliases'] = array_merge(isset($aliases) ? $aliases : array(), isset($options['site-aliases']) ? $options['site-aliases'] : array());
        unset($options['site-aliases']);
        $options['command-specific'] = array_merge(isset($command_specific) ? $command_specific : array(), isset($options['command-specific']) ? $options['command-specific'] : array());

        mash_set_config_options($context, $options, $override);
      }
    }
  }
}

function mash_set_config_options($context, $options, $override = array()) {
  // Copy 'config-file' into 'context-path', converting to an array to hold multiple values if necessary
  if (isset($options['config-file'])) {
    if (isset($options['context-path'])) {
      $options['context-path'] = array_merge(array($options['config-file']), is_array($options['context-path']) ? $options['context-path'] : array($options['context-path']));
    }
    else {
      $options['context-path'] = $options['config-file'];
    }
  }

  // Take out $aliases and $command_specific options
  mash_set_config_special_contexts($options);

  mash_set_context($context, $options);
}

/**
 * For all global options with a short form, convert all options in the option
 * array that use the short form into the long form.
 */
function mash_expand_short_form_options(&$options) {
  foreach (mash_get_global_options() as $name => $info) {
    if (is_array($info)) {
      // For any option with a short form, check to see if the short form was set in the
      // options.  If it was, then rename it to its long form.
      if (array_key_exists('short-form', $info) && array_key_exists($info['short-form'], $options)) {
        if (!array_key_exists($name, $options) || !array_key_exists('merge-pathlist', $info)) {
          $options[$name] = $options[$info['short-form']];
        }
        else {
          $options[$name] = array_merge((array)$options[$name], (array)$options[$info['short-form']]);
        }
        unset($options[$info['short-form']]);
      }
    }
  }
}

/**
 * There are certain options such as 'site-aliases' and 'command-specific'
 * that must be merged together if defined in multiple mash configuration
 * files.  If we did not do this merge, then the last configuration file
 * that defined any of these properties would overwrite all of the options
 * that came before in previously-loaded configuration files.  We place
 * all of them into their own context so that this does not happen.
 */
function mash_set_config_special_contexts(&$options) {
  if (isset($options) && is_array($options)) {
    $has_command_specific = array_key_exists('command-specific', $options);
    // Change the keys of the site aliases from 'alias' to '@alias'
    if (array_key_exists('site-aliases', $options)) {
      $user_aliases = $options['site-aliases'];
      $options['site-aliases'] = array();
      foreach ($user_aliases as $alias_name => $alias_value) {
        if (substr($alias_name,0,1) != '@') {
          $alias_name = "@$alias_name";
        }
        $options['site-aliases'][$alias_name] = $alias_value;
      }
    }
    // Expand -s into --simulate, etc.
    mash_expand_short_form_options($options);

    foreach (mash_get_global_options() as $name => $info) {
      if (is_array($info)) {
        // For any global option with the 'merge-pathlist' or 'merge-associative' flag, set its
        // value in the specified context.  The option is 'merged' because we
        // load $options with the value from the context prior to including the
        // configuration file.  If the configuration file sets $option['special'][] = 'value',
        // then the configuration will be merged.  $option['special'] = array(...), on the
        // other hand, will replace rather than merge the values together.
        if ((array_key_exists($name, $options)) && (array_key_exists('merge', $info) || (array_key_exists('merge-pathlist', $info) || array_key_exists('merge-associative', $info)))) {
          $context = array_key_exists('context', $info) ? $info['context'] : $name;
          $cache =& mash_get_context($context);
          $value = $options[$name];
          if (!is_array($value) && array_key_exists('merge-pathlist', $info)) {
            $value = explode(PATH_SEPARATOR, $value);
          }
          if (array_key_exists('merge-associative', $info)) {
            foreach ($value as $subkey => $subvalue) {
              $cache[$subkey] = array_merge(isset($cache[$subkey]) ? $cache[$subkey] : array(), $subvalue);
            }
          }
          else {
            $cache = array_unique(array_merge($cache, $value));
          }
          // Once we have moved the option to its special context, we
          // can remove it from its option context -- unless 'propagate-cli-value'
          // is set, in which case we need to let it stick around in options
          // in case it is needed in backend invoke.
          if (!array_key_exists('propagate-cli-value', $info)) {
            unset($options[$name]);
          }
        }
      }
    }

    // If command-specific options were set and if we already have
    // a command, then apply the command-specific options immediately.
    if ($has_command_specific) {
      mash_command_default_options();
    }
  }
}

/**
 * Set a specific context.
 *
 * @param context
 *   Any of the default defined contexts.
 * @param value
 *   The value to store in the context
 *
 * @return
 *   An associative array of the settings specified in the request context.
 */
function mash_set_context($context, $value) {
  $cache =& mash_get_context($context);
  $cache = $value;
  return $value;
}


/**
 * Return a specific context, or the whole context cache
 *
 * This function provides a storage mechanism for any information
 * the currently running process might need to communicate.
 *
 * This avoids the use of globals, and constants.
 *
 * Functions that operate on the context cache, can retrieve a reference
 * to the context cache using :
 *     $cache = &mash_get_context($context);
 *
 * This is a private function, because it is meant as an internal
 * generalized API for writing static cache functions, not as a general
 * purpose function to be used inside commands.
 *
 * Code that modifies the reference directly might have unexpected consequences,
 * such as modifying the arguments after they have already been parsed and dispatched
 * to the callbacks.
 *
 * @param context
 *   Optional. Any of the default defined contexts.
 *
 * @return
 *   If context is not supplied, the entire context cache will be returned.
 *   Otherwise only the requested context will be returned.
 *   If the context does not exist yet, it will be initialized to an empty array.
 */
function &mash_get_context($context = NULL, $default = NULL) {
  static $cache = array();
  if (!is_null($context)) {
    if (!isset($cache[$context])) {
      $default = is_null($default) ? array() : $default;
      $cache[$context] = $default;
    }
    return $cache[$context];
  }
  return $cache;
}

/**
 * Set the arguments passed to the mash.php script.
 *
 * This function will set the 'arguments' context of the current running script.
 *
 * When initially called by mash_parse_args, the entire list of arguments will
 * be populated. Once the command is dispatched, this will be set to only the remaining
 * arguments to the command (i.e. the command name is removed).
 *
 * @param arguments
 *   Command line arguments, as an array.
 */
function mash_set_arguments($arguments) {
  mash_set_context('arguments', $arguments);
}

/**
 * Get the arguments passed to the mash.php script.
 *
 * When mash_set_arguments is initially called by mash_parse_args,
 * the entire list of arguments will be populated.
 * Once the command has been dispatched, this will be return only the remaining
 * arguments to the command.
 */
function mash_get_arguments() {
  return mash_get_context('arguments');
}

/**
 * Set the command being executed.
 *
 * Mash_dispatch will set the correct command based on it's
 * matching of the script arguments retrieved from mash_get_arguments
 * to the implemented commands specified by mash_get_commands.
 *
 * @param
 *   A numerically indexed array of command components.
 */
function mash_set_command($command) {
  mash_set_context('command', $command);
}

/**
 * Return the command being executed.
 *
 *
 */
function mash_get_command() {
  return mash_get_context('command');
}
/**
 * Get the value for an option.
 *
 * If the first argument is an array, then it checks whether one of the options
 * exists and return the value of the first one found. Useful for allowing both
 * -h and --host-name
 *
 * @param option
 *   The name of the option to get
 * @param default
 *   Optional. The value to return if the option has not been set
 * @param context
 *   Optional. The context to check for the option. If this is set, only this context will be searched.
 */
function mash_get_option($option, $default = NULL, $context = NULL) {
  $value = NULL;

  if ($context) {
    // We have a definite context to check for the presence of an option.
    $value = _mash_get_option($option, mash_get_context($context));
  }
  else {
    // We are not checking a specific context, so check them in a predefined order of precedence.
    $contexts = mash_context_names();

    foreach ($contexts as $context) {
      $value = _mash_get_option($option, mash_get_context($context));

      if ($value !== NULL) {
        return $value;
      }
    }
  }

  if ($value !== NULL) {
    return $value;
  }

  return $default;
}

/**
 * Get the value for an option and return it as a list.  If the
 * option in question is passed on the command line, its value should
 * be a comma-separated list (e.g. --flag=1,2,3).  If the option
 * was set in a mashrc.php file, then its value may be either a
 * comma-separated list or an array of values (e.g. $option['flag'] = array('1', '2', '3')).
 *
 * @param option
 *   The name of the option to get
 * @param default
 *   Optional. The value to return if the option has not been set
 * @param context
 *   Optional. The context to check for the option. If this is set, only this context will be searched.
 */
function mash_get_option_list($option, $default = array(), $context = NULL) {
  $result = mash_get_option($option, $default, $context);

  if (!is_array($result)) {
    $result = explode(',', $result);
  }

  return $result;
}

/**
 * Get the value for an option, but first checks the provided option overrides.
 *
 * The feature of mash_get_option that allows a list of option names
 * to be passed in an array is NOT supported.
 *
 * @param option_overrides
 *   An array to check for values before calling mash_get_option.
 * @param option
 *   The name of the option to get.
 * @param default
 *   Optional. The value to return if the option has not been set.
 * @param context
 *   Optional. The context to check for the option. If this is set, only this context will be searched.
 *
 */
function mash_get_option_override($option_overrides, $option, $default = NULL, $context = NULL) {
  return mash_sitealias_get_option($option_overrides, $option, $default, '', $context);
}

/**
 * Get an option out of the specified alias.  If it has not been
 * set in the alias, then get it via mash_get_option.
 *
 * @param site_alias_record
 *   An array of options for an alias record.
 * @param option
 *   The name of the option to get.
 * @param default
 *   Optional. The value to return if the option does not exist in the site record and has not been set in a context.
 * @param context
 *   Optional. The context to check for the option. If this is set, only this context will be searched.
 */
function mash_sitealias_get_option($site_alias_record, $option, $default = NULL, $prefix = '', $context = NULL) {
  if (is_array($site_alias_record) && array_key_exists($option, $site_alias_record)) {
    return $site_alias_record[$option];
  }
  else {
    return mash_get_option($prefix . $option, $default, $context);
  }
}

/**
 * Get all of the values for an option in every context.
 *
 * @param option
 *   The name of the option to get
 * @return
 *   An array whose key is the context name and value is
 *   the specific value for the option in that context.
 */
function mash_get_context_options($option, $flatten = FALSE) {
  $result = array();

  $contexts = mash_context_names();
  foreach ($contexts as $context) {
    $value = _mash_get_option($option, mash_get_context($context));

    if ($value !== NULL) {
      if ($flatten && is_array($value)) {
        $result = array_merge($value, $result);
      }
      else {
        $result[$context] = $value;
      }
    }
  }

  return $result;
}

/**
 * Retrieves a collapsed list of all options.
 */
function mash_get_merged_options() {
  $contexts = mash_context_names();
  $cache = mash_get_context();
  $result = array();
  foreach (array_reverse($contexts) as $context) {
    if (array_key_exists($context, $cache)) {
      $result = array_merge($result, $cache[$context]);
    }
  }

  return $result;
}

/**
 * Retrieves a collapsed list of all options
 * with a specified prefix.
 */
function mash_get_merged_prefixed_options($prefix) {
  $merged_options = mash_get_merged_options();
  $result = array();
  foreach ($merged_options as $key => $value) {
    if ($prefix == substr($key, 0, strlen($prefix))) {
      $result[substr($key, strlen($prefix))] = $value;
    }
  }

  return $result;
}

/**
 * Helper function to recurse through possible option names
 */
function _mash_get_option($option, $context) {
  if (is_array($option)) {
    foreach ($option as $current) {
      if (array_key_exists($current, $context)) {
        return $context[$current];
      }
    }
  }
  elseif (array_key_exists($option, $context)) {
    return $context[$option];
  }

  return NULL;
}

/**
 * Set an option in one of the option contexts.
 *
 * @param option
 *   The option to set.
 * @param value
 *   The value to set it to.
 * @param context
 *   Optional. Which context to set it in.
 * @return
 *   The value parameter. This allows for neater code such as
 *     $myvalue = mash_set_option('http_host', $_SERVER['HTTP_HOST']);
 *   Without having to constantly type out the value parameter.
 */
function mash_set_option($option, $value, $context = 'process') {
  $cache =& mash_get_context($context);
  $cache[$option] = $value;
  return $value;
}

/**
 * A small helper function to set the value in the default context
 */
function mash_set_default($option, $value) {
  return mash_set_option($option, $value, 'default');
}

/**
 * Remove a setting from a specific context.
 *
 * @param
 *   Option to be unset
 * @param
 *   Context in which to unset the value in.
 */
function mash_unset_option($option, $context = NULL) {
  if ($context != NULL) {
    $cache =& mash_get_context($context);
    if (array_key_exists($option, $cache)) {
      unset($cache[$option]);
    }
  }
  else {
    $contexts = mash_context_names();

    foreach ($contexts as $context) {
      mash_unset_option($option, $context);
    }
  }
}

/**
 * Save the settings in a specific context to the applicable configuration file
 * This is useful is you want certain settings to be available automatically the next time a command is executed.
 *
 * @param $context
 *   The context to save
 */
function mash_save_config($context) {
  $filename = _mash_config_file($context);
  if (is_array($filename)) {
    $filename = $filename[0];
  }

  if ($filename) {
    $cache = mash_get_context($context);

    $fp = fopen($filename, "w+");
    if (!$fp) {
      return mash_set_error('MASH_PERM_ERROR', dt('Mashrc (!filename) could not be written', array('!filename' => $filename)));
    }
    else {
      fwrite($fp, "<?php\n");
      $timestamp = mktime();
      foreach ($cache as $key => $value) {
        $line = "\n\$options['$key'] = ". var_export($value, TRUE) .';';
        fwrite($fp, $line);
      }
      fwrite($fp, "\n");
      fclose($fp);
      mash_log(dt('Mashrc file (!filename) was written successfully', array('!filename' => $filename)));
      return TRUE;
    }

  }
  return FALSE;
}

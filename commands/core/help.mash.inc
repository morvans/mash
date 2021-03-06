<?php

/**
 * Prints out help for a given command.
 */
function mash_show_help($commandstring) {
  // First check and see if the command can already be found.
  $commands = mash_get_commands();
  if (!array_key_exists($commandstring, $commands)) {
    // If the command cannot be found, then bootstrap so that
    // additional commands will be brought in.
    // For speed, only bootstrap up to MASH_BOOTSTRAP_MAGENTO_SITE.
    mash_bootstrap_max(MASH_BOOTSTRAP_MAGENTO_SITE);
    $commands = mash_get_commands();
  }
  if (array_key_exists($commandstring, $commands)) {
    $command = $commands[$commandstring];
    mash_print_help($command);
    return TRUE;
  }
  $shell_aliases = mash_get_context('shell-aliases', array());
  if (array_key_exists($commandstring, $shell_aliases)) {
    $msg = dt("'@alias-name' is a shell alias.  Its value is: !name. See `mash topic docs-shell-aliases` and `mash shell-alias` for more information.", array('@alias-name' => $commandstring, '!name' => $shell_aliases[$commandstring]));
    mash_log($msg, 'ok');
    return TRUE;
  }
  return mash_set_error('MASH_COMMAND_NOT_FOUND', dt('Invalid command !command.', array('!command' => $commandstring)));
}

/**
 * Print the help for a single command to the screen.
 *
 * @param array $command
 *   A fully loaded $command array.
 */
function mash_print_help($command) {
  _mash_help_merge_subcommand_information($command);

  if (!$help = mash_command_invoke_all('mash_help', 'mash:'. $command['command'])) {
    $help = array($command['description']);
  }

  if ($command['strict-option-handling']) {
    $command['topics'][] = 'docs-strict-options';
  }

  // Give commandfiles an opportunity to add examples and options to the command.
  mash_bootstrap_max(MASH_BOOTSTRAP_MAGENTO_SITE);
  mash_command_invoke_all_ref('mash_help_alter', $command);

  mash_print(wordwrap(implode("\n", $help), mash_get_context('MASH_COLUMNS', 80)));
  mash_print();

  // Sort command options.
  uksort($command['options'], '_mash_help_sort_command_options');

  // Print command sections help.
  foreach ($command['sections'] as $key => $value) {
    if (!empty($command[$key])) {
      mash_print(dt($value) . ':');
      $rows = mash_format_help_section($command, $key);
      mash_print_table($rows, FALSE, array(40));
      unset($rows);
      mash_print();
    }
  }

  // Append aliases if any.
  if ($command['aliases']) {
    mash_print(dt("Aliases: ") . implode(', ', $command['aliases']));
  }
}

/**
 * Sort command options alphabetically. Engine options at the end.
 */
function _mash_help_sort_command_options($a, $b) {
  $engine_a = strpos($a, '=');
  $engine_b = strpos($b, '=');
  if ($engine_a && !$engine_b) {
    return 1;
  }
  else if (!$engine_a && $engine_b) {
    return -1;
  }
  elseif ($engine_a && $engine_b) {
    if (substr($a, 0, $engine_a) == substr($b, 0, $engine_b)) {
      return 0;
    }
  }
  return ($a < $b) ? -1 : 1;
}

/**
 * Check to see if the specified command contains an 'allow-additional-options'
 * record.  If it does, find the additional options that are allowed, and
 * add in the help text for the options of all of the sub-commands.
 */
function _mash_help_merge_subcommand_information(&$command) {
  // 'allow-additional-options' will either be FALSE (default),
  // TRUE ("allow anything"), or an array that lists subcommands
  // that are or may be called via mash_invoke by this command.
  if (is_array($command['allow-additional-options'])) {
    $implemented = mash_get_commands();
    foreach ($command['allow-additional-options'] as $subcommand_name) {
      if (array_key_exists($subcommand_name, $implemented)) {
        $command['options'] += $implemented[$subcommand_name]['options'];
        $command['sub-options'] = array_merge_recursive($command['sub-options'], $implemented[$subcommand_name]['sub-options']);
        if (empty($command['arguments'])) {
          $command['arguments'] = $implemented[$subcommand_name]['arguments'];
        }
        $command['topics'] = array_merge($command['topics'], $implemented[$subcommand_name]['topics']);
      }
    }
  }
}

/**
 * Format one named help section from a command record
 *
 * @param $command
 *   A command record with help information
 * @param $section
 *   The name of the section to format ('options', 'topic', etc.)
 * @returns array
 *   Formatted rows, suitable for printing via mash_print_table.
 */
function mash_format_help_section($command, $section) {
  $formatter = (function_exists('mash_help_section_formatter_' . $section)) ? 'mash_help_section_formatter_' . $section : 'mash_help_section_default_formatter';
  foreach ($command[$section] as $name => $help_attributes) {
    if (!is_array($help_attributes)) {
      $help_attributes = array('description' => $help_attributes);
    }
    $help_attributes['label'] = $name;
    call_user_func_array($formatter, array($command, &$help_attributes));
    if (!array_key_exists('hidden', $help_attributes)) {
      $rows[] = array($help_attributes['label'], $help_attributes['description']);
      // Process the subsections too, if any
      if (!empty($command['sub-' . $section]) && array_key_exists($name, $command['sub-' . $section])) {
        $rows = array_merge($rows, _mash_format_help_subsection($command, $section, $name, $formatter));
      }
    }
  }
  return $rows;
}

/**
 * Format one named portion of a subsection from a command record.
 * Subsections allow related parts of a help record to be grouped
 * together.  For example, in the 'options' section, sub-options that
 * are related to a particular primary option are stored in a 'sub-options'
 * section whose name == the name of the primary option.
 *
 * @param $command
 *   A command record with help information
 * @param $section
 *   The name of the section to format ('options', 'topic', etc.)
 * @param $subsection
 *   The name of the subsection (e.g. the name of the primary option)
 * @param $formatter
 *   The name of a function to use to format the rows of the subsection
 * @param $prefix
 *   Characters to prefix to the front of the label (for indentation)
 * @returns array
 *   Formatted rows, suitable for printing via mash_print_table.
 */
function _mash_format_help_subsection($command, $section, $subsection, $formatter, $prefix = '  ') {
  foreach ($command['sub-' . $section][$subsection] as $name => $help_attributes) {
    if (!is_array($help_attributes)) {
      $help_attributes = array('description' => $help_attributes);
    }
    $help_attributes['label'] = $name;
    call_user_func_array($formatter, array($command, &$help_attributes));
    if (!array_key_exists('hidden', $help_attributes)) {
      $rows[] = array($prefix . $help_attributes['label'], $help_attributes['description']);
      // Process the subsections too, if any
      if (!empty($command['sub-' . $section]) && array_key_exists($name, $command['sub-' . $section])) {
        $rows = array_merge($rows, _mash_format_help_subsection($command, $section, $name, $formatter, $prefix . '  '));
      }
    }
  }
  return $rows;
}

/**
 * The options section formatter.  Adds a "--" in front of each
 * item label.  Also handles short-form and example-value
 * components in the help attributes.
 */
function mash_help_section_formatter_options($command, &$help_attributes) {
  if ($help_attributes['label'][0] == '-') {
    mash_log(dt("Option '!option' of command !command should instead be declared as '!fixed'", array('!option' => $help_attributes['label'], '!command' => $command['command'], '!fixed' => preg_replace('/^--*/', '', $help_attributes['label']))), 'debug');
  }
  else {
    $help_attributes['label'] = '--' . $help_attributes['label'];
  }
  if (array_key_exists('required', $help_attributes)) {
    $help_attributes['description'] .= " " . dt("Required.");
  }

  $prefix = '<';
  $suffix = '>';
  if (array_key_exists('example-value', $help_attributes)) {
    if (isset($help_attributes['value']) && $help_attributes['value'] == 'optional') {
      $prefix = '[';
      $suffix = ']';
    }
    $help_attributes['label'] .= '=' . $prefix . $help_attributes['example-value'] . $suffix;

    if (array_key_exists('short-form', $help_attributes)) {
      $help_attributes['short-form'] .= " $prefix" . $help_attributes['example-value'] . $suffix;
    }
  }
  if (array_key_exists('short-form', $help_attributes)) {
    $help_attributes['label'] = '-' . $help_attributes['short-form'] . ', ' . $help_attributes['label'];
  }
  mash_help_section_default_formatter($command, $help_attributes);
}

/**
 * The default section formatter.  Replaces '[command]' with the
 * command name.
 */
function mash_help_section_default_formatter($command, &$help_attributes) {
  // '[command]' is a token representing the current command. @see pm_mash_engine_version_control().
  $help_attributes['label'] = str_replace('[command]', $command['command'], $help_attributes['label']);
}

/**
 * Build a fake command for the purposes of showing examples and options.
 */
function mash_global_options_command($brief = FALSE) {
  $global_options_help = array(
    'description' => 'Execute a mash command. Run `mash help [command]` to view command-specific help.  Run `mash topic` to read even more documentation.',
    'sections' => array(
      'options' => 'Global options (see `mash topic core-global-options` for the full list)',
    ),
    'options' => mash_get_global_options($brief),
    'examples' => array(
      'mash dl cck zen' => 'Download CCK module and Zen theme.',
      'mash --uri=http://example.com status' => 'Show status command for the example.com multi-site.',
    ),
    '#brief' => TRUE,
  );
  $global_options_help += mash_command_defaults('global-options', 'global_options', __FILE__);
  mash_command_invoke_all_ref('mash_help_alter', $global_options_help);

  return $global_options_help;
}

/**
 * Command callback for help command. This is the default command, when none
 * other has been specified.
 */
function mash_core_help() {
  $commands = func_get_args();

  if (empty($commands)) {
    // For speed, only bootstrap up to MASH_BOOTSTRAP_MAGENTO_CONFIGURATION.
    mash_bootstrap_max(MASH_BOOTSTRAP_MAGENTO_CONFIGURATION);
    $implemented = mash_get_commands();

    // Organize all commands into categories
    $command_categories = array();
    $category_map = array();
    foreach ($implemented as $key => $candidate) {
      if ((!array_key_exists('is_alias', $candidate) || !$candidate['is_alias']) && !$candidate['hidden']) {
        $category = $candidate['commandfile'];
        // If we have decided to remap a category, remap every command
        if (array_key_exists($category, $category_map)) {
          $category = $category_map[$category];
        }
        if (!array_key_exists($category, $command_categories)) {
          $title = mash_command_invoke_all('mash_help', "meta:$category:title");
          $alternate_title = '';
          if (!$title) {
            // If there is no title, then check to see if the
            // command file is stored in a folder with the same
            // name as some other command file (e.g. 'core') that
            // defines a title.
            $alternate = basename($candidate['path']);
            $alternate_title = mash_command_invoke_all('mash_help', "meta:$alternate:title");
          }
          if (!empty($alternate_title)) {
            $category_map[$category] = $alternate;
            $category = $alternate;
            $title = $alternate_title;
          }
          $command_categories[$category]['title'] = empty($title) ? '' : $title[0];
          $summary = mash_command_invoke_all('mash_help', "meta:$category:summary");
          if ($summary) {
            $command_categories[$category]['summary'] = $summary[0];
          }
        }
        $command_categories[$category]['commands'][] = $key;
      }
    }

    // Sort the command categories; make sure that 'core' is
    // always first in the list
    $core_category = array('core' => $command_categories['core']);
    unset($command_categories['core']);
    // Post-process the categories that have no title.
    // Any that have fewer than 4 commands will go into a
    // section called "other".
    $processed_categories = array();
    $misc_categories = array();
    $other_commands = array();
    $other_categories = array();
    foreach ($command_categories as $key => $info) {
      if (empty($info['title'])) {
        $one_category = $key;
        if (count($info['commands']) < 4) {
          $other_commands = array_merge($other_commands, $info['commands']);
          $other_categories[] = $one_category;
        }
        else {
          $info['title'] = dt("All commands in !category", array('!category' => $key));
          $misc_categories[$one_category] = $info;
        }
      }
      else {
        $processed_categories[$key] = $info;
      }
    }
    $other_category = array();
    if (!empty($other_categories)) {
      $other_category[implode(',', $other_categories)] = array('title' => dt("Other commands"), 'commands' => $other_commands);
    }
    asort($processed_categories);
    asort($misc_categories);
    $command_categories = array_merge($core_category, $processed_categories, $misc_categories, $other_category);

    // If the user specified --filter w/out a value, then
    // present a list of help options.
    if (mash_get_option('filter', FALSE) === TRUE) {
      $help_categories = array();
      foreach ($command_categories as $key => $info) {
        $description = $info['title'];
        if (array_key_exists('summary', $info)) {
          $description .= ": " . $info['summary'];
        }
        $help_categories[$key] = $description;
      }
      $result = mash_choice($help_categories, 'Select a help category:');
      if (!$result) {
        return mash_user_abort();
      }
      mash_set_option('filter', $result);
    }

    if (mash_get_option('html')) {
      mash_print(mash_help_html_header());
    }

    // Make a fake command section to hold the global options, then print it.
    $global_options_help = mash_global_options_command(TRUE);
    if ((!mash_get_option('html')) && (!mash_get_option('filter'))) {
      mash_print_help($global_options_help);
    }

    // Filter out categories that the user does not want to see
    if ($filter_category = mash_get_option('filter')) {
      if (!array_key_exists($filter_category, $command_categories)) {
        return mash_set_error('MASH_NO_CATEGORY', dt("The specified command category !filter does not exist.", array('!filter' => $filter_category)));
      }
      $command_categories = array($filter_category => $command_categories[$filter_category]);
    }

    // If the user specified --sort, then merge all of the remaining
    // categories together
    if (mash_get_option('sort', FALSE)) {
      $combined_commands = array();
      foreach ($command_categories as $key => $info) {
        $combined_commands = array_merge($combined_commands, $info['commands']);
      }
      $command_categories = array('all' => array('commands' => $combined_commands, 'title' => dt("Commands:")));
    }

    // Next, print out the table of commands by category.
    $all_commands = array();
    foreach ($command_categories as $key => $info) {
      // Get the commands in this category and sort them
      $commands = $info['commands'];
      sort($commands);

      // Remove hidden commands and build output for mash_print_table.
      $rows = array();
      foreach($commands as $cmd) {
        $command = $implemented[$cmd];
        $all_commands[$cmd] = $command;
        $name = $command['aliases'] ? $cmd . ' (' . implode(', ', $command['aliases']) . ')': $cmd;
        $rows[$cmd] = array($name, $command['description']);
        $pipe[] = $cmd;
      }

      // Vary the output by mode: text or html
      if (mash_get_option('html')) {
        $sorted_commands = array();
        foreach($commands as $cmd) {
          $sorted_commands[$cmd] = $implemented[$cmd];
        }

        mash_print("<h3>" . $info['title'] . "</h3>");
        mash_print(mash_help_html_command_list($sorted_commands));
      }
      else {
        mash_print($info['title'] . ": (" . $key . ")");
        mash_print_table($rows, FALSE, array(0 => 20));
      }
    }

    // Print out the long-form help for all commands
    if (mash_get_option('html')) {
      mash_print(mash_help_html_global_options($global_options_help));
      mash_print(mash_help_html($all_commands));
    }

    // Newline-delimited list for use by other scripts. Set the --pipe option.
    if (mash_get_option('pipe') && isset($pipe)) {
      sort($pipe);
      mash_print_pipe(implode(" ", $pipe));
    }
    return;
  }
  else {
    $result = TRUE;
    while ((count($commands) > 0) && !mash_get_error()) {
      $result =  mash_show_help(array_shift($commands));
    }
    return $result;
  }

  return mash_set_error('MASH_COMMAND_NOT_FOUND', dt('Invalid command !command.', array('!command' => implode(" ", $commands))));
}


/**
 * Return an HTML page header that includes all global options.
 */
function mash_help_html_header() {
  $output =  "<html><head><title>mash help</title><style>dt {font-size: 110%; font-weight: bold}</style></head><body>\n";
}

function mash_help_html_global_options($global_options_help) {
  // Global options
  $global_option_rows = mash_format_help_section($global_options_help, 'options');
  $output =  '<h3>Global Options (see `mash topic core-global-options` for the full list)</h3><table>';
  foreach ($global_option_rows as $row) {
    $output .= "<tr>";
    foreach ($row as $value) {
      $output .=  "<td>" . htmlspecialchars($value) . "</td>\n";
    }
    $output .= "</tr>";
  }
  $output .=  "</table>\n";

  return $output;
}

function mash_help_html_command_list($commands) {
  // Command table
  $output =  "<table>\n";
  foreach ($commands as $key => $command) {
    $output .=  "  <tr><td><a href=\"#$key\">$key</a></td><td>" . $command['description'] . "</td></tr>\n";
  }
  $output .=  "</table>\n";

  return $output;
}

/**
 * Return an HTML page documenting all available commands and global options.
 */
function mash_help_html($commands) {
  // Command details
  $output =  '<h3>Command detail</h3><dl>';
  foreach ($commands as $key => $command) {
    $output .=  "\n<a name=\"$key\"></a><dt>$key</dt><dd><pre>\n";
    ob_start();
    mash_show_help($key);
    $output .= ob_get_clean();
    $output .=  "</pre></dd>\n";
  }

  $output .=  "</body></html>\n";

  return $output;
}

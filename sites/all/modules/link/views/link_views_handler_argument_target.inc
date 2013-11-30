<?php

/**
 * @file
 * Argument handler to filter results by target.
 */

/**
 * Argument handler to filter results by target.
 */
class link_views_handler_argument_target extends views_handler_argument {

  /**
   * Provide a default options form for the argument.
   */
  function options_form(&$form, &$form_state) {
    $defaults = $this->default_actions();

    $form['title'] = array(
      '#prefix' => '<div class="clear-block">',
      '#suffix' => '</div>',
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#default_value' => $this->options['title'],
      '#description' => t('The title to use when this argument is present; it will override the title of the view and titles from previous arguments. You can use percent substitution here to replace with argument titles. Use "%1" for the first argument, "%2" for the second, etc.'),
    );

    $form['clear_start'] = array(
      '#value' => '<div class="clear-block">',
    );

    $form['defaults_start'] = array(
      '#value' => '<div class="views-left-50">',
    );

    $form['default_action'] = array(
      '#type' => 'radios',
      '#title' => t('Action to take if argument is not present'),
      '#default_value' => $this->options['default_action'],
    );

    $form['defaults_stop'] = array(
      '#value' => '</div>',
    );

    $form['wildcard'] = array(
      '#prefix' => '<div class="views-right-50">',
      // prefix and no suffix means these two items will be grouped together.
      '#type' => 'textfield',
      '#title' => t('Wildcard'),
      '#size' => 20,
      '#default_value' => $this->options['wildcard'],
      '#description' => t('If this value is received as an argument, the argument will be ignored; i.e, "all values"'),
    );

    $form['wildcard_substitution'] = array(
      '#suffix' => '</div>',
      '#type' => 'textfield',
      '#title' => t('Wildcard title'),
      '#size' => 20,
      '#default_value' => $this->options['wildcard_substitution'],
      '#description' => t('The title to use for the wildcard in substitutions elsewhere.'),
    );

    $form['clear_stop'] = array(
      '#value' => '</div>',
    );

    $options = array();
    $validate_options = array();
    foreach ($defaults as $id => $info) {
      $options[$id] = $info['title'];
      if (empty($info['default only'])) {
        $validate_options[$id] = $info['title'];
      }
      if (!empty($info['form method'])) {
        $this->{$info['form method']}($form, $form_state);
      }
    }

    $form['default_action']['#options'] = $options;

    $form['validate_type'] = array(
      '#type' => 'select',
      '#title' => t('Validator'),
      '#default_value' => $this->options['validate_type'],
    );

    $validate_types = array('none' => t('<Basic validation>'));
    $plugins = views_fetch_plugin_data('argument validator');
    foreach ($plugins as $id => $info) {
      $valid = TRUE;
      if (!empty($info['type'])) {
        $valid = FALSE;
        if (empty($this->definition['validate type'])) {
          continue;
        }
        foreach ((array) $info['type'] as $type) {
          if ($type == $this->definition['validate type']) {
            $valid = TRUE;
            break;
          }
        }
      }

      // If we decide this validator is ok, add it to the list.
      if ($valid) {
        $plugin = views_get_plugin('argument validator', $id);
        if ($plugin) {
          $plugin->init($this->view, $this, $id);
          if ($plugin->access()) {
            $plugin->validate_form($form, $form_state, $id);
            $validate_types[$id] = $info['title'];
          }
        }
      }
    }

    asort($validate_types);
    $form['validate_type']['#options'] = $validate_types;
    // Show this gadget if *anything* but 'none' is selected

    $form['validate_fail'] = array(
      '#type' => 'select',
      '#title' => t('Action to take if argument does not validate'),
      '#default_value' => $this->options['validate_fail'],
      '#options' => $validate_options,
    );
  }

  /**
   * Set up the query for this argument.
   *
   * The argument sent may be found at $this->argument.
   */
  function query() {
    $this->ensure_my_table();
    // Because attributes are stored serialized, our only option is to also
    // serialize the data we're searching for and use LIKE to find similar data.
    $this->query->add_where(0, $this->table_alias .'.'. $this->real_field ." LIKE '%%%s%'", serialize(array('target' => $this->argument)));
  }
}

<?php
/**
 * @file
 * Support for migrate module
 */

class MigrateLinkFieldHandler extends MigrateFieldHandler {
  public function __construct() {
    $this->registerTypes(array('link'));
  }

  static function arguments($title = NULL, $attributes = NULL, $language = NULL) {
    $arguments = array();
    if (!is_null($title)) {
      $arguments['title'] = $title;
    }
    if (!is_null($attributes)) {
      $arguments['attributes'] = $attributes;
    }
    if (!is_null($language)) {
      $arguments['language'] = $language;
    }
    return $arguments;
  }

  public function prepare($entity, array $instance, array $values) {
    if (isset($values['arguments'])) {
      $arguments = $values['arguments'];
      unset($values['arguments']);
    }
    else {
      $arguments = array();
    }

    $delta = 0;
    foreach($values as $value) {
      $item = array();
      if (isset($arguments['title'])) {
        $item['title'] = $arguments['title'];
      }
      if (isset($arguments['attributes'])) {
        $item['attributes'] = $arguments['attributes'];
      }
      $item['url'] = $value;
      $return[$delta] = $item;
      $delta++;
    }

    return isset($return) ? $return : NULL;
  }
}

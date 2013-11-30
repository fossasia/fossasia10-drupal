<?php

/**
 * @file
 * Theme functions for the link module.
 */

/**
 * Theme the settings form for the link field.
 */
function theme_link_field_settings($form) {
  $title_value = drupal_render($form['title_value']);
  $title_checkbox = drupal_render($form['title']['value']);

  // Set Static Title radio option to include the title_value textfield.
  $form['title']['value'] = array('#value' => '<div class="container-inline">'. $title_checkbox . $title_value .'</div>');

  // Reprint the title radio options with the included textfield.
  return drupal_render($form);
}

/**
 * FAPI theme for an individual text elements.
 */
function theme_link($element) {
  drupal_add_css(drupal_get_path('module', 'link') .'/link.css');

  // Prefix single value link fields with the name of the field.
  if (empty($element['#field']['multiple'])) {
    if (isset($element['url']) && isset($element['title'])) {
      $element['url']['#title'] = $element['#title'] .' '. $element['url']['#title'];
      $element['title']['#title'] = $element['#title'] .' '. $element['title']['#title'];
    }
    elseif ($element['url']) {
      $element['url']['#title'] = $element['#title'];
    }
  }

  $output = '';
  $output .= '<div class="link-field-subrow clear-block">';
  if (isset($element['title'])) {
    $output .= '<div class="link-field-title link-field-column">'. theme('textfield', $element['title']) .'</div>';
  }
  $output .= '<div class="link-field-url'. (isset($element['title']) ? ' link-field-column' : '') .'">'. theme('textfield', $element['url']) .'</div>';
  $output .= '</div>';
  if (!empty($element['attributes']['target'])) {
    $output .= '<div class="link-attributes">'. theme('checkbox', $element['attributes']['target']) .'</div>';
  }
  return $output;
}

/**
 * Theme function for 'default' text field formatter.
 */
function theme_link_formatter_default($element) {
  // Display a normal link if both title and URL are available.
  if (!empty($element['#item']['display_title']) && !empty($element['#item']['url'])) {
    return l($element['#item']['display_title'], $element['#item']['url'], $element['#item']);
  }
  // If only a title, display the title.
  elseif (!empty($element['#item']['display_title'])) {
    return check_plain($element['#item']['display_title']);
  }
}

/**
 * Theme function for 'plain' text field formatter.
 */
function theme_link_formatter_plain($element) {
  return empty($element['#item']['url']) ? check_plain($element['#item']['title']) : url($element['#item']['url'], $element['#item']);
}

/**
 * Theme function for 'absolute' link field formatter.
 */
function theme_link_formatter_absolute($element) {
  $absolute = array('absolute' => TRUE);
  return empty($element['#item']['url']) ? '' : url($element['#item']['url'], $absolute + $element['#item']);
}

/**
 * Theme function for 'title_plain' text field formatter.
 */
function theme_link_formatter_title_plain($element) {
  return empty($element['#item']['title']) ? '' : check_plain($element['#item']['title']);
}

/**
 * Theme function for 'url' text field formatter.
 */
function theme_link_formatter_url($element) {
  return $element['#item']['url'] ? l($element['#item']['display_url'], $element['#item']['url'], $element['#item']) : '';
}

/**
 * Theme function for 'short' text field formatter.
 */
function theme_link_formatter_short($element) {
  return $element['#item']['url'] ? l(t('Link'), $element['#item']['url'], $element['#item']) : '';
}

/**
 * Theme function for 'label' text field formatter.
 */
function theme_link_formatter_label($element) {
  return $element['#item']['url'] ? l($element['#item']['label'], $element['#item']['url'], $element['#item']) : '';
}

/**
 * Theme function for 'separate' text field formatter.
 */
function theme_link_formatter_separate($element) {
  $class = empty($element['#item']['attributes']['class']) ? '' : ' '. $element['#item']['attributes']['class'];
  unset($element['#item']['attributes']['class']);
  $title = empty($element['#item']['title']) ? '' : check_plain($element['#item']['title']);

  $output = '';
  $output .= '<div class="link-item '. $class .'">';
  if (!empty($title)) {
    $output .= '<div class="link-title">'. $title .'</div>';
  }
  $output .= '<div class="link-url">'. l($element['#item']['display_url'], $element['#item']['url'], $element['#item']) .'</div>';
  $output .= '</div>';
  return $output;
}

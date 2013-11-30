<?php

/**
 * @file
 * Helper functions for Link field, widget and form elements.
 */

function _link_load($field, &$items) {
  foreach ($items as $delta => $item) {
    // Unserialize the attributes array.
    $items[$delta]['attributes'] = unserialize($item['attributes']);
  }
  return array($field['field_name'] => $items);
}

function _link_process(&$item, $delta = 0, $field, $node) {
  // Trim whitespace from URL.
  $item['url'] = trim($item['url']);

  // if no attributes are set then make sure $item['attributes'] is an empty array - this lets $field['attributes'] override it.
  if (empty($item['attributes'])) {
    $item['attributes'] = array();
  }

  // Serialize the attributes array.
  $item['attributes'] = serialize($item['attributes']);

  // Don't save an invalid default value (e.g. 'http://').
  if ((isset($field['widget']['default_value'][$delta]['url']) && $item['url'] == $field['widget']['default_value'][$delta]['url']) && is_object($node)) {
    if (!link_validate_url($item['url'])) {
      unset($item['url']);
    }
  }
}

function _link_validate(&$item, $delta, $field, $node, &$optional_field_found) {
  if ($item['url'] && !(isset($field['widget']['default_value'][$delta]['url']) && $item['url'] == $field['widget']['default_value'][$delta]['url'] && !$field['required'])) {
    // Validate the link.
    if (link_validate_url(trim($item['url'])) == FALSE) {
      form_set_error($field['field_name'] .']['. $delta .'][url', t('Not a valid URL.'));
    }
    // Require a title for the link if necessary.
    if ($field['title'] == 'required' && strlen(trim($item['title'])) == 0) {
      form_set_error($field['field_name'] .']['. $delta .'][title', t('Titles are required for all links.'));
    }
  }
  // Require a link if we have a title.
  if ($field['form_id'] != 'content_field_edit_form' && $field['url'] !== 'optional' && strlen($item['title']) > 0 && strlen(trim($item['url'])) == 0) {
    form_set_error($field['field_name'] .']['. $delta .'][url', t('You cannot enter a title without a link url.'));
  }
  // In a totally bizzaro case, where URLs and titles are optional but the field is required, ensure there is at least one link.
  if ($field['url'] == 'optional' && $field['title'] == 'optional' && (strlen(trim($item['url'])) != 0 || strlen(trim($item['title'])) != 0)) {
    $optional_field_found = TRUE;
  }
}

/**
 * Cleanup user-entered values for a link field according to field settings.
 *
 * @param $item
 *   A single link item, usually containing url, title, and attributes.
 * @param $delta
 *   The delta value if this field is one of multiple fields.
 * @param $field
 *   The CCK field definition.
 * @param $node
 *   The node containing this link.
 */
function _link_sanitize(&$item, $delta, &$field, &$node) {
  // Don't try to process empty links.
  if (empty($item['url']) && empty($item['title']) && empty($field['title_value'])) {
    return;
  }

  // Replace URL tokens.
  if ($field['enable_tokens'] && module_exists('token')) {
    // Load the node if necessary for nodes in views.
    $token_node = isset($node->nid) ? node_load($node->nid) : $node;
    $item['url'] = token_replace($item['url'], 'node', $token_node);
  }

  $type = link_validate_url($item['url']);
  // If we can't determine the type of url, and we've been told not to validate it,
  // then we assume it's a LINK_EXTERNAL type for later processing. #357604
  if ($type == FALSE && $field['validate_url'] === 0) {
    $type = LINK_EXTERNAL;
  }
  $url = link_cleanup_url($item['url']);

  // Separate out the anchor if any.
  if (strpos($url, '#') !== FALSE) {
    $item['fragment'] = substr($url, strpos($url, '#') + 1);
    $url = substr($url, 0, strpos($url, '#'));
  }
  // Separate out the query string if any.
  if (strpos($url, '?') !== FALSE) {
    $item['query'] = substr($url, strpos($url, '?') + 1);
    $url = substr($url, 0, strpos($url, '?'));
  }
  // Save the new URL without the anchor or query.
  if ($field['validate_url'] === 0) {
    $item['url'] = check_plain($url);
  }
  else {
    $item['url'] = $url;
  }

  // Create a shortened URL for display.
  $display_url = $type == LINK_EMAIL ? str_replace('mailto:', '', $url) : url($url, array('query' => isset($item['query']) ? $item['query'] : NULL, 'fragment' => isset($item['fragment']) ? $item['fragment'] : NULL, 'absolute' => TRUE));
  if (is_array($field['display']) && isset($field['display']['url_cutoff']) && strlen($display_url) > $field['display']['url_cutoff']) {
    $display_url = substr($display_url, 0, $field['display']['url_cutoff']) ."...";
  }
  $item['display_url'] = $display_url;

  // Use the title defined at the field level.
  if ($field['title'] == 'value' && strlen(trim($field['title_value']))) {
    $title = $field['title_value'];
  }
  // Use the title defined by the user at the widget level.
  else {
    $title = $item['title'];
  }
  // Replace tokens. - originally we only did it for value titles.
  if (($field['title'] == 'value' || $field['enable_tokens']) && module_exists('token')) {
    // Load the node if necessary for nodes in views.
    $token_node = isset($node->nid) ? node_load($node->nid) : $node;
    $title = filter_xss(token_replace($title, 'node', $token_node), array('b', 'br', 'code', 'em', 'i', 'img', 'span', 'strong', 'sub', 'sup', 'tt', 'u'));
    $item['html'] = TRUE;
  }
  elseif ($field['title'] == 'value') {
    $title = filter_xss($title, array('b', 'br', 'code', 'em', 'i', 'img', 'span', 'strong', 'sub', 'sup', 'tt', 'u'));
    $item['html'] = TRUE;
  }
  $item['display_title'] = empty($title) ? $item['display_url'] : $title;

  if (!isset($item['attributes'])) {
    $item['attributes'] = array();
  }

  // Unserialize attributtes array if it has not been unserialized yet.
  if (!is_array($item['attributes'])) {
    $item['attributes'] = (array)unserialize($item['attributes']);
  }

  // Add default attributes.  Make sure that $field['attributes'] is an array. #626932
  if (!is_array($field['attributes'])) {
    $field['attributes'] = array();
  }
  $field['attributes'] += _link_default_attributes();

  // Merge item attributes with attributes defined at the field level.
  $item['attributes'] = array_filter($item['attributes']);
  $item['attributes'] += $field['attributes'];

  // If user is not allowed to choose target attribute, use default defined at
  // field level.
  if ($field['attributes']['target'] != LINK_TARGET_USER) {
    $item['attributes']['target'] = $field['attributes']['target'];
  }

  // Remove the target attribute if the default (no target) is selected.
  if (empty($item['attributes']) || $item['attributes']['target'] == LINK_TARGET_DEFAULT) {
    unset($item['attributes']['target']);
  }

  // Remove the rel=nofollow for internal links.
  if ($type != LINK_EXTERNAL && $type != LINK_NEWS && strpos($item['attributes']['rel'], 'nofollow') !== FALSE) {
    $item['attributes']['rel'] = str_replace('nofollow', '', $item['attributes']['rel']);
  }
  // Some old field data may have $item['#item']['attributes']['rel'] as an array, which will cause errors:
  if (isset($item['#item']['attributes']['rel']) && is_array($item['#item']['attributes']['rel'])) {
    unset($item['#item']['attributes']['rel']);
  }

  // Handle "title" link attribute
  if (!empty($item['attributes']['title']) && module_exists('token')) {
    // Load the node (necessary for nodes in views).
    $token_node = isset($node->nid) ? node_load($node->nid) : $node;
    $item['attributes']['title'] = token_replace($item['attributes']['title'], 'node', $token_node);
  }

  // Remove title attribute if it's equal to link text.
  if ($item['attributes']['title'] == $item['display_title']) {
    unset($item['attributes']['title']);
  }

  // Remove empty attributes.
  $item['attributes'] = array_filter($item['attributes']);

  // Add the widget label.
  $item['label'] = $field['widget']['label'];
}

function _link_default_attributes() {
  return array(
    'title' => '',
    'target' => LINK_TARGET_DEFAULT,
    'class' => '',
    'rel' => '',
  );
}

/**
 * Forms a valid URL if possible from an entered address.
 * Trims whitespace and automatically adds an http:// to addresses without a protocol specified
 *
 * @param string $url
 * @param string $protocol The protocol to be prepended to the url if one is not specified
 */
function link_cleanup_url($url, $protocol = "http") {
  $url = trim($url);
  $type = link_validate_url($url);

  if ($type == LINK_EXTERNAL) {
    // Check if there is no protocol specified.
    $protocol_match = preg_match("/^([a-z0-9][a-z0-9\.\-_]*:\/\/)/i", $url);
    if (empty($protocol_match)) {
      // But should there be? Add an automatic http:// if it starts with a domain name.
      $domain_match = preg_match('/^(([a-z0-9]([a-z0-9\-_]*\.)+)('. LINK_DOMAINS .'|[a-z]{2}))/i', $url);
      if (!empty($domain_match)) {
        $url = $protocol ."://". $url;
      }
    }
  }

  return $url;
}

/**
 * Wrapper around html_entity_decode to handle problems with PHP 4.
 *
 * See http://drupal.org/node/739650
 * See http://bugs.php.net/bug.php?id=25670
 *
 * We've taken this away from the beginning of file define() step, as this is
 * going to be slower for PHP4, and we don't want to run this on every page load,
 * just when we're doing a validate.
 */
function _link_html_entity_decode($html_string, $quote_style = ENT_COMPAT, $charset) {
  if (defined('PHP_VERSION')) {
    $version = explode('.', PHP_VERSION);
    if ($version[0] == '5') {
      return html_entity_decode($html_string, $quote_style, $charset); // PHP 5, use default.
    }
  }
  else {
    $version = explode('.', PHP_VERSION);
    if ($version[0] == '5') {
      return html_entity_decode($html_string, $quote_style, $charset);
    }
  }
  // use suggested code from http://drupal.org/node/739650
  // replace numeric entities
  $string = preg_replace('~&#x([0-9a-f]+);~ei', "_link_code2utf(hexdec('\\1'))", $html_string);
  $string = preg_replace('~&#([0-9]+);~e', '_link_code2utf("\\1")', $string);

  // replace literal entities.
  $trans_tbl = get_html_translation_table(HTML_ENTITIES);
  $trans_tbl = array_flip($trans_tbl);

  return strtr($string, $trans_tbl);
}

/**
 * Returns the utf string corresponding to the unicode value.
 *
 * Needed for handling utf characters in PHP4.
 *
 * @see http://www.php.net/manual/en/function.html-entity-decode.php#75153
 */
function _link_code2utf($num) {
  if ($num < 128) return chr($num);
  if ($num < 2048) return chr(($num >> 6) + 192) . chr(($num & 63) + 128);
  if ($num < 65536) return chr(($num >> 12) + 224) . chr((($num >> 6) & 63) + 128) . chr(($num & 63) + 128);
  if ($num < 2097152) return chr(($num >> 18) + 240) . chr((($num >> 12) & 63) + 128) . chr((($num >> 6) & 63) + 128) . chr(($num & 63) + 128);
  return '';
}

/**
 * A lenient verification for URLs. Accepts all URLs following RFC 1738 standard
 * for URL formation and all email addresses following the RFC 2368 standard for
 * mailto address formation.
 *
 * @param string $text
 * @return mixed Returns boolean FALSE if the URL is not valid. On success, returns an object with
 * the following attributes: protocol, hostname, ip, and port.
 */
function link_validate_url($text) {
  $LINK_ICHARS_DOMAIN = (string) _link_html_entity_decode(implode("", array(
    "&#x00E6;", // æ
    "&#x00C6;", // Æ
    "&#x00F8;", // ø
    "&#x00D8;", // Ø
    "&#x00E5;", // å
    "&#x00C5;", // Å
    "&#x00E4;", // ä
    "&#x00C4;", // Ä
    "&#x00F6;", // ö
    "&#x00D6;", // Ö
    "&#x00FC;", // ü
    "&#x00DC;", // Ü
    "&#x00D1;", // Ñ
    "&#x00F1;", // ñ
  )), ENT_QUOTES, 'UTF-8');

  $LINK_ICHARS = $LINK_ICHARS_DOMAIN . (string) _link_html_entity_decode(implode("", array(
    "&#x00DF;", // ß
  )), ENT_QUOTES, 'UTF-8');
  $allowed_protocols = variable_get('filter_allowed_protocols', array('http', 'https', 'ftp', 'news', 'nntp', 'telnet', 'mailto', 'irc', 'ssh', 'sftp', 'webcal'));

  $protocol = '(('. implode("|", $allowed_protocols) .'):\/\/)';
  $authentication = '(([a-z0-9%' . $LINK_ICHARS . ']+(:[a-z0-9%'. $LINK_ICHARS . '!]*)?)?@)';
  $domain = '(([a-z0-9' . $LINK_ICHARS_DOMAIN . ']([a-z0-9'. $LINK_ICHARS_DOMAIN . '\-_\[\]])*)(\.(([a-z0-9' . $LINK_ICHARS_DOMAIN . '\-_\[\]])+\.)*('. LINK_DOMAINS .'|[a-z]{2}))?)';
  $ipv4 = '([0-9]{1,3}(\.[0-9]{1,3}){3})';
  $ipv6 = '([0-9a-fA-F]{1,4}(\:[0-9a-fA-F]{1,4}){7})';
  $port = '(:([0-9]{1,5}))';

  // Pattern specific to external links.
  $external_pattern = '/^'. $protocol .'?'. $authentication .'?('. $domain .'|'. $ipv4 .'|'. $ipv6 .' |localhost)'. $port .'?';

  // Pattern specific to internal links.
  $internal_pattern = "/^([a-z0-9". $LINK_ICHARS ."_\-+\[\]]+)";
  $internal_pattern_file = "/^([a-z0-9". $LINK_ICHARS ."_\-+\[\]\.]+)$/i";

  $directories = "(\/[a-z0-9". $LINK_ICHARS ."_\-\.~+%=&,$'!():;*@\[\]]*)*";
  // Yes, four backslashes == a single backslash.
  $query = "(\/?\?([?a-z0-9". $LINK_ICHARS ."+_|\-\.\/\\\\%=&,$'():;*@\[\]{} ]*))";
  $anchor = "(#[a-z0-9". $LINK_ICHARS ."_\-\.~+%=&,$'():;*@\[\]\/\?]*)";

  // The rest of the path for a standard URL.
  $end = $directories .'?'. $query .'?'. $anchor .'?'.'$/i';

  $message_id = '[^@].*@'. $domain;
  $newsgroup_name = '([0-9a-z+-]*\.)*[0-9a-z+-]*';
  $news_pattern = '/^news:('. $newsgroup_name .'|'. $message_id .')$/i';

  $user = '[a-zA-Z0-9'. $LINK_ICHARS .'_\-\.\+\^!#\$%&*+\/\=\?\`\|\{\}~\'\[\]]+';
  $email_pattern = '/^mailto:'. $user .'@'.'('. $domain .'|'. $ipv4 .'|'. $ipv6 .'|localhost)'. $query .'?$/';

  if (strpos($text, '<front>') === 0) {
    return LINK_FRONT;
  }
  if (in_array('mailto', $allowed_protocols) && preg_match($email_pattern, $text)) {
    return LINK_EMAIL;
  }
  if (in_array('news', $allowed_protocols) && preg_match($news_pattern, $text)) {
    return LINK_NEWS;
  }
  if (preg_match($internal_pattern . $end, $text)) {
    return LINK_INTERNAL;
  }
  if (preg_match($external_pattern . $end, $text)) {
    return LINK_EXTERNAL;
  }
  if (preg_match($internal_pattern_file, $text)) {
    return LINK_INTERNAL;
  }

  return FALSE;
}

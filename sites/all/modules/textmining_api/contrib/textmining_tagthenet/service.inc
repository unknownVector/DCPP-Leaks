<?php

/**
 * @file
 * Service classes.
 */

/**
 * Text Minig service class for Tagthe.net Terms.
 */
class TextminingApiServiceTagthenet extends TextminingApiServiceAbstract {
  
  public function getMetadataList() {
    return array(
      'topic' => array('name' => t('Topic'), 'type' => 'text'),
      'title' => array('name' => t('Title'), 'type' => 'text'),
      'person' => t('Person'),
      'language' => t('language'),
      'author' => array('name' => t('Author'), 'type' => 'text'),
    );
  }
  
  public function threadGenerateItemTerms(ProcessApiThread $thread, $id, array $item) {
    
    $text='';
    foreach ($item as $field) {
        if (!is_string($field['value'])) continue;
       $text .= ' ' . $field['value'];
    }

    $options = array(
      'data' => 'text=' . urlencode($text),
      'method' => 'POST',
    );
  
    $response = textmining_api_http_request(TEXTMINING_TAGTHENET_URL, $options);
    if ($response->code != 200) {
      watchdog('autotagging_tagthenet', 'Received response code !code from Tagthe.net', array('!code' => $response->code), WATCHDOG_ERROR);
      return FALSE;
    }
    
    $dom = DOMDocument::loadXML($response->data);
    $xp = new DOMXPath($dom);
    $terms=array();
    
    $dims = $xp->evaluate("/memes/meme/dim");
    for ($i = 0; $i < $dims->length; $i++) {
      $dim = $dims->item($i);
      $type = $dim->getAttribute("type");
      if ($field = $thread->getMetadataOption($type)) {
        
        $terms[$field]['add_mode'] = $thread->combineMetadataOption(
            $type, 
            'add_mode', 
            isset($terms[$field]['add_mode']) ? $terms[$field]['add_mode'] : NULL);

        $terms[$field]['allow_create_terms'] = $thread->combineMetadataOption(
            $type, 
            'allow_create_terms', 
            isset($terms[$field]['allow_create_terms']) ? $terms[$field]['allow_create_terms'] : NULL);

        $items = $xp->evaluate("item", $dim);
        for ($j = 0; $j < $items->length; $j++) {
          $dim_item = $items->item($j);
          $terms[$field]['datas'][] = $dim_item->nodeValue;
        }
      }
    }
    return $terms;
  }

  
}

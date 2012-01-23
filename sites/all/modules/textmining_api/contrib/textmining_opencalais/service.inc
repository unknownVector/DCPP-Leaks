<?php

/**
 * @file
 * Service classes.
 */

/**
 * Text Minig service class for OpenCalais Terms.
 */
class TextminingApiServiceOpenCalais extends TextminingApiServiceAbstract {
  
  public function configurationForm(array $form, array &$form_state) {
    $form=parent::configurationForm($form, $form_state);
    
    $form += array(
      'opencalais_apikey' => array(
        '#type' => 'textfield',
        '#title' => t('OpenCalais API Key'),
        '#default_value' => isset($this->server->options['custom']['opencalais_apikey']) ? $this->server->options['custom']['opencalais_apikey'] : NULL,
        '#required' => TRUE,
      ),
    );
        
    return $form;
  }

  public function threadGenerateItemTerms(ProcessApiThread $thread, $id, array $item) {
    
    $text='';
    foreach ($item as $field) {
        if (!is_string($field['value'])) continue;
       $text .= ' ' . $field['value'];
    }

    $data = array();
    $xmlinput = '<c:params xmlns:c="http://s.opencalais.com/1/pred/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"><c:processingDirectives c:contentType="text/html" c:enableMetadataType="GenericRelations,SocialTags" c:outputFormat="Application/JSON"/></c:params>';
    $data['licenseID'] = $thread->server()->options['custom']['opencalais_apikey'];
    $data['content'] = $text;
    $data['paramsXML'] = $xmlinput;
      
    $options = array(
      'data' => $data,
      'method' => 'POST',
    );
    
    $response = textmining_api_http_request(TEXTMINING_OPENCALAIS_URL, $options);
    if ($response->code != 200) {
      watchdog('texmining_opencalais', 'Received response code !code from OpenCalais', array('!code' => $response->code), WATCHDOG_ERROR);
      return FALSE;
    }
    
    $terms=array();
    
    $tags=json_decode($response->data, TRUE);
    if (is_array($tags))
    {
      foreach($tags as $tag)
      {
        if (!isset($tag['name'])) continue;
        $field=FALSE;
        if (isset($tag['_type'])) {
          // specific tag type
          $type=$tag['_type'];
          if ($field = $thread->getMetadataOption($type)) {
            
            $terms[$field]['add_mode'] = $thread->combineMetadataOption(
                $type, 
                'add_mode', 
                isset($terms[$field]['add_mode']) ? $terms[$field]['add_mode'] : NULL);

            $terms[$field]['allow_create_terms'] = $thread->combineMetadataOption(
                $type, 
                'allow_create_terms', 
                isset($terms[$field]['allow_create_terms']) ? $terms[$field]['allow_create_terms'] : NULL);

          }
        }
        if (!$field) {
          //take default values
          if ($field = $thread->getMetadataOption('default')) {
            $terms[$field]['add_mode'] = $thread->combineMetadataOption(
                'default', 
                'add_mode', 
                isset($terms[$field]['add_mode']) ? $terms[$field]['add_mode'] : NULL);

            $terms[$field]['allow_create_terms'] = $thread->combineMetadataOption(
                'default', 
                'allow_create_terms', 
                isset($terms[$field]['allow_create_terms']) ? $terms[$field]['allow_create_terms'] : NULL);
          }
        }
        if ($field) {
          $terms[$field]['datas'][] = $tag['name'];
        }
      }
    }
    return $terms;
  }
  
  public function getMetadataList() {
    return array(
      'default' => array(), // just a default conf 
      'City' => array(),
      'Company' => array(),
      'Continent' => array(),
      'Country' => array(),
      'Currency' => array('type' => 'text'),
      'EmailAddress' => array('type' => 'text'),
      'FaxNumber' => array('type' => 'text'),
      'MarketIndex' => array('type' => 'text'),
      'NaturalFeature' => array(),
      'Organization' => array(),
      'Person' => array(),
      'PhoneNumber' => array('type' => 'text'),
      'ProvinceOrState' => array(),
      'Region' => array(),
      'URL' => array('type' => 'text'),
    );
  }
}

<?php

/**
 * @file
 * Service classes.
 */

/**
 * Process service abstract class for Text mining API.
 */
abstract class TextminingApiServiceAbstract extends ProcessApiServiceAbstract implements TextminingApiServiceInterface {
  
  public function threadConfigurationForm(ProcessApiThread $thread, array $form, array &$form_state) {
    
    $entity_type = $thread->entity_type?$thread->entity_type:'node';
    
    $tagops = array('' => t('None'));
    $textops = array('' => t('None'));
    foreach (field_info_instances($entity_type) as $fields) {
      foreach ($fields as $field => $field_info) {
        $label = $field_info['label'];
        $field_info = field_info_field($field);
        if (strpos($field_info['type'], 'text')!==FALSE) {
          $tagops[$field] = $label;
          $textops[$field] = $label;
        }
        if ( $field_info['type'] == 'taxonomy_term_reference' ) {
          // look up for taxonomy fields
          $vocab=taxonomy_vocabulary_machine_name_load($field_info['settings']['allowed_values'][0]['vocabulary']);
          if ($vocab) $tagops[$field] = t('@field (Taxonomy @vocab)', array('@vocab' => $vocab->name, '@field' => $label));
        }
      }
    }
    
    $form = array();
    
    foreach ($this->getMetadataList() as $meta => $meta_info) {
      if (!is_array($meta_info)) {
        $meta_info = array('name' => $meta_info);
      }
      if (!isset($meta_info['name'])) {
        $meta_info['name'] = $meta;
      }
      if (!isset($meta_info['type'])) {
        $meta_info['type'] = 'tag';
      }
       
      $form['field:' . $meta] = array(
          '#type' => 'select',
          '#title' => t('Target field for @meta', array('@meta' => $meta_info['name'])),
          '#description' => t('Metadatas will be added to this field.'),
          '#options' => $meta_info['type'] == 'tag' ? $tagops : $textops,
          '#default_value' => isset($thread->options['custom']['field:' . $meta]) ? $thread->options['custom']['field:' . $meta] : NULL,
        );
        
      if ($meta_info['type'] == 'tag') {
        $form['allow_create_terms:' . $meta] = array(
            '#type' => 'checkbox',
            '#title' => t('Allow create terms'),
            '#description' => t('Terms will be created if needed.'),
            '#default_value' => isset($thread->options['custom']['allow_create_terms:' . $meta]) ? $thread->options['custom']['allow_create_terms:' . $meta] : TRUE,
          );
      }

      $form['add_mode:' . $meta] = array(
          '#type' => 'select',
          '#title' => t('Add mode'),
          '#description' => t('The way Text Mining API will add data.'),
          '#options' => array('empty' => t('Only if no existing data'), 'append' => t('Append new data'), 'reset' => t('Reset before adding')),
          '#default_value' => isset($thread->options['custom']['add_mode:' . $meta]) ? $thread->options['custom']['add_mode:' . $meta] : 'empty',
        );
        
      $form['sep:' . $meta] = array(
        '#markup' => '<hr>',
      );  
        
    }
    
    return $form;
  }  

  public function threadProcessItem(ProcessApiThread $thread, $id, array $item, $context) {
    if ($context['mode']=='delete') return TRUE; 
    $add_terms_mode = isset($thread->options['custom']['add_terms_mode']) ? $thread->options['custom']['add_terms_mode'] : 'empty';
    $allow_create_terms = isset($thread->options['custom']['allow_create_terms']) ? $thread->options['custom']['allow_create_terms'] : TRUE;
    $item_entity=entity_load($thread->entity_type, array($id));
    $item_entity=reset($item_entity);
    
    if (!$item_entity) {
      watchdog('texmining_api', 'Cannot load entity !entity with !id', array('!entity' => $thread->entity_type, '!id' => $id), WATCHDOG_ERROR);
      return FALSE;
    }
    
    $metas = $this->getMetadataList();
    $terms = $this->threadGenerateItemTerms($thread, $id, $item);
    
    if (!is_array($terms)) {
      // FALSE means errors, if no terms should return an empty array.
      return FALSE;
    }
    
    // no datas
    if (empty($terms)) return TRUE;
    
    if (!is_array(reset($terms))) {
     
      // we have just a single list of terms
      if ( count($metas) == 1 && isset($thread->options['custom']['field:' . reset(array_keys($metas))]) ) {
        $terms = array( $thread->options['custom']['field:' . reset(array_keys($metas))] => $terms );
      }
      else {
        watchdog('texmining_api', 'Cannot determinate field to populate for thread !thread', array('!thread' => $thread->machine_name), WATCHDOG_ERROR);
        return FALSE;
      }
    }
    
    $modified=FALSE;
    
    foreach ($terms as $field => $field_terms) {
      if (!isset($field_terms['datas'])) {
        $field_terms = array('datas' => $field_terms);
      }
      if (!isset($field_terms['add_mode'])) {
        $field_terms['add_mode'] = 'empty';
      }
      if (!isset($field_terms['allow_create_terms'])) {
        $field_terms['allow_create_terms'] = FALSE;
      }

      if (isset($item_entity->{$field})) {
        
        $field_info = field_info_field($field);
        if (field_is_translatable($thread->entity_type, $field_info)) {
          $available_languages=field_available_languages($thread->entity_type, $field);
          $langcode = language_default('language');
          if (!in_array($langcode, $available_languages)) {
            // fallback to undefined
            $langcode = LANGUAGE_NONE;
          }
        }
        else {
          $langcode = LANGUAGE_NONE;
        }

        switch ($field_terms['add_mode']) {
          case 'reset':
            $item_entity->{$field}[$langcode]=array();
            break;
          case 'append':
            //nothing
            break;
          default: //empty
            if (isset($item_entity->{$field}[$langcode])&&count($item_entity->{$field}[$langcode])) {
              //nothing
              continue;
            }
        }
        
        if ($field_info['type'] == 'taxonomy_term_reference') {
          $vocabulary=taxonomy_vocabulary_machine_name_load($field_info['settings']['allowed_values'][0]['vocabulary']);
          foreach (array_unique($field_terms['datas']) as $data) {
            $terms=taxonomy_term_load_multiple(array(), array('name' => trim($data), 'vid' => $vocabulary->vid));
            $term=reset($terms);
            if ((!$term)&&$field_terms['allow_create_terms']) {
              $term=(object)array(
                  'vid' => $vocabulary->vid,
                  'name' => $data,
                );
              taxonomy_term_save($term);
            }
            
            if ($term) {
              $modified=TRUE;
              $item_entity->{$field}[$langcode][]=array('tid' => $term->tid);
            }
          }
        }
        else {
          foreach (array_unique($field_terms['datas']) as $data) {
            $modified=TRUE;
            $item_entity->{$field}[$langcode][] = array('value' => trim($data));
          }
        }
      }
    }
    
    if ($modified) {
      entity_save($thread->entity_type, $item_entity);
    }
    
    return TRUE;
  }
  
  /**
   * Helpers
   */
  public function threadGetMetadataOption(ProcessApiThread $thread, $meta, $name='field') {
    if (isset($thread->options['custom'][$name . ':' . $meta])) return $thread->options['custom'][$name . ':' . $meta];
    return NULL;
  }
  
  public function threadCombineMetadataOption(ProcessApiThread $thread, $meta, $name, $current = NULL) {  
    $new = $this->threadGetMetadataOption($thread, $meta, $name);
    
    if ($current===NULL) {
      return $new;   
    }
    if ($current==$new) return $new;
    switch ($name) {
      case 'add_mode':
        if ($current=='empty'||$new=='empty') {
          return 'empty';
        }
        if ($current=='append'||$new=='append')
        {
          return 'append';
        }
        return 'reset';
      case 'allow_create_terms':
        return $current&&$new;
    }
  }
  
  /**
   * By default we handle only nodes.
   * @see ProcessApiServiceAbstract::getSupportedEntityTypes()
   */
  public function getSupportedEntityTypes() {
    // arbitrary supports only nodes
    return array('node' => entity_get_info('node'));
  }
}

/**
 * Process service interface for Text mining API.
 */
interface TextminingApiServiceInterface {
  
  /**
   * Return the terms for an entity.
   * @param ProcessApiThread $thread
   * @param integer $id
   * @param array $item
   * 
   * @return array|FALSE
   *   The generated terms or FALSE if errors (will be re-processed)
   * 
   * for lonely term ref field :
   *   array(term1, term2, ...)
   *   
   * for multiple fields and specific options:
   *   array(
   *     field1 => array('datas' => array(data1, data2, ...), 'add_mode' => 'empty'|'append'|'reset', 'allow_create_terms' => TRUE|FALSE),
   *     field2 => array(data1, data2, ...),
   *     ...
   *   )
   */
  public function threadGenerateItemTerms(ProcessApiThread $thread, $id, array $item);
  
  /**
   * List the metadatas provided by this service.
   * 
   *  @return array|FALSE
   *  array(
   *  	'meta1' => array('name' => 'Meta 1', 'type' => 'tag|text'),
   *  	'meta2' => 'Meta 2', // default type is tag
   *  )
   */
  public function getMetadataList();
}
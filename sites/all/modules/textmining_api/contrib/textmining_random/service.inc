<?php

/**
 * @file
 * Service classes.
 */

/**
 * Text Minig service class for Random Terms.
 */
class TextminingApiServiceRandom extends TextminingApiServiceAbstract {
  
  public function threadConfigurationForm(ProcessApiThread $thread, array $form, array &$form_state) {
    $form = parent::threadConfigurationForm($thread, $form, $form_state);
    $form += array(
      'number' => array(
        '#type' => 'textfield',
        '#size' => 5,
        '#title' => t('Number of terms'),
        '#description' => t('Number of terms to create +/- 50%.'),
        '#default_value' => isset($thread->options['custom']['number']) ? $thread->options['custom']['number'] : 5,
      ),
    );
    return $form;
  }
  
  public function getMetadataList() {
    return array(
      'random' => t('Random terms'),
    );
  }
  
  public function threadGenerateItemTerms(ProcessApiThread $thread, $id, array $item) {
    module_load_include('inc', 'devel_generate');
    
    $n = isset($thread->options['custom']['number']) ? $thread->options['custom']['number'] : 5;
    
    $terms = array_chunk(explode(' ', ucwords(substr(devel_create_greeking($n), 0, -1))), rand($n*0.5, $n*1.5));
    
    return reset($terms);
  }
}

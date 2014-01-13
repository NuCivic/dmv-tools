<?php

/**
* Implementation of hook_drush_command().
*/
function rsttools_drush_command() {

  $examples = array('drush <command> show' => 'all show nodes.',
      'drush <command> show,blog_post' => 'all nodes of type show and blog_post.',
      'drush <command> 1,4,6' => 'test node ids 1, 4 and 6.',
      'drush <command> 1-10' => 'test node ids in the range 1-10.',
      'drush <command> 1,7,10-12,article,98' => 'nodes with node id 1, 7, in the range of 10 to 12, 98 and all nodes of type article.');

  $items['export-publish-status'] = array(
    'callback' => '_rsttools_export_publish_status',
    'drupal dependencies' => array('node'),
    'description' => dt('write the current publish status to a drupal file'),
    'aliases' => array('rstxps'),
    'examples' => $examples,
    'arguments' => array(
      'argument-list' => "A comma seperated list of node types, nids or nid ranges to export.",
      'name' => "a name to give to the unmanaged exported file. Name only, no paths. Saves to drupal's public file directory.",
    ),
  );

  $items['import-publish-status'] = array(
    'callback' => '_rsttools_import_publish_status',
    'drupal dependencies' => array('node'),
    'description' => dt('restore the publish status based on the previously exported publish list'),
    'aliases' => array('rstips'),
    'examples' => $examples,
    'arguments' => array(
      'name' => "name of the file to restore",
    ),
  );

  $items['stress-publish'] = array(
    'callback' => '_rsttools_stress_publish',
    'drupal dependencies' => array('node'),
    'description' => dt('Stress test by repeatedly publishing and unpublishing nodes'),
    'aliases' => array('rstsp'),
    'examples' => $examples,
    'arguments' => array(
      'argument-list' => "A comma seperated list of node types, nids or nid ranges to test.",
      'duration' => "time to run the test in SECONDS",
      'sleep-time' => "sleep time between publish flips in MILLISECONDS",
    ),
  );

  $items['stress-cache'] = array(
    'callback' => '_rsttools_stress_cache',
    'drupal dependencies' => array('node'),
    'description' => dt('Stress test by repeatedly resetting the cache'),
    'aliases' => array('rstsc'),
    'examples' => $examples,
    'arguments' => array(
      'argument-list' => "A comma seperated list of node types, nids or nid ranges to stress test.",
      'duration' => "time to run the test in SECONDS",
      'sleep-time' => "sleep time between publish flips in MILLISECONDS",
    ),
  );

  $items['publish-nodes'] = array(
    'callback' => '_rsttools_publish',
    'drupal dependencies' => array('node'),
    'description' => dt('Publish nodes'),
    'aliases' => array('rstpn'),
    'examples' => $examples,
    'arguments' => array(
      'argument-list' => "A comma seperated list of node types, nids or nid ranges to publish.",
    ),
  );

  $items['unpublish-nodes'] = array(
    'callback' => '_rsttools_unpublish',
    'drupal dependencies' => array('node'),
    'description' => dt('Unpublish nodes'),
    'aliases' => array('rstupn'),
    'examples' => $examples,
    'arguments' => array(
      'argument-list' => "A comma seperated list of node types, nids or nid ranges to unpublish.",
    ),
  );

  $items['delete-users'] = array(
    'callback' => '_rsttools_delete_users',
    'drupal dependencies' => array('node'),
    'description' => dt('delete users'),
    'aliases' => array('rstdu'),
  );

  return $items;
}

//test function
function _rsttools_delete_users() {


    $query = db_select('users', 'u');
    $query->fields('u', array('status','created','name'));
    $result = $query->execute();
    $nu = 0;
    while($r = $result->fetchAssoc()) {
         drush_print(dt("created: @c status: @s name: @n", array("@c"=> format_date($r['created'],"small"), "@s" => $r["status"], "@n" => $r["name"])));
	 ++$nu;
    }

    drush_print(dt("total users: @nu", array("@nu"=>$nu)));
}

function _rsttools_export_publish_status($arg, $name) {

	$nids = _rsttools_parse_node_argument_list($arg);

	$data = array();

	$t = $f = 0; //number of success/failures

  	foreach($nids as $nid) {

        	if ($node = node_load($nid)) {
	        	$data[$nid] = $node->status;
			++$t;
		} else {
			drush_print(dt("could not export node @nid. does not exist.", array("@nid" => $nid)));
			++$f;
		}
	}

	file_unmanaged_save_data(json_encode($data), "public://rsttools-".$name, $replace = FILE_EXISTS_REPLACE);

	drush_print(dt("exported $t nodes. could not export $f nodes."));
}

function _rsttools_import_publish_status($name) {

	if (!file_destination($uri, FILE_EXISTS_ERROR)) {

		$filename = "public://rsttools-$name";
		$handle = fopen($filename, "r");
		$strData = fread($handle, filesize($filename));
		fclose($handle);

	        $nids = json_decode($strData);

	        $t = $f = 0; //number of success/failures

	        foreach($nids as $nid=>$status) {

                  if ($node = node_load($nid)) {
			if ($node->status!=$status) {
				$node->status = $status;
                        	++$t;
			} else {
				++$f;
			}
                  } else {
                        drush_print(dt("could not export node @nid. does not exist.", array("@nid" => $nid)));
                        ++$f;
                  }
          }

          drush_print(dt("reset node status on $t nodes. $f were untouched. file $name"));

	} else {

	  drush_print(dt("could not load file $name. Nothing to do."));
	}
}

function _rsttools_stress_cache($arg, $duration, $sleep) {

	$nids = _rsttools_parse_node_argument_list($arg);

	$time = time();

	//run for $duration
  	while (time()<$time+$duration) {

		//get a random nid to cache bust
		$nid = _rsttools_random_nid($nids);

		//reset cache
		entity_get_controller('node')->resetCache(array($nid));

		drush_print(dt('cleared cache for node @nid', array('@nid' => $nid)));

		//sleep for required millseconds
	        usleep($sleep*1000);
	}
}

function _rsttools_stress_publish($arg, $duration, $sleep) {

  $touchedNids = array();

  drush_print(dt('running test for @duration seconds. Do not abort script or nodes may be left in incorrect status.', array('@duration' => $duration)));

  $nids = _rsttools_parse_node_argument_list($arg);

  $time = time();

  //run for $duration
  while (time()<$time+$duration) {

    $nid = _rsttools_random_nid($nids);

    $node = node_load($nid);

    //write out the existing nid/status to a file for safety
    if (!array_key_exists($nid, $touchedNids)) {

	//remember original node status
	$touchedNids[$nid] = $node->status;
	drush_print(dt('logging original node @nid status @status.', array('@nid' => $nid, '@status' => $node->status)));
	//write to file nid and status
    }

    if ($node->status == NODE_PUBLISHED) $node->status = NODE_NOT_PUBLISHED;
    elseif ($node->status == NODE_NOT_PUBLISHED) $node->status = NODE_PUBLISHED;

    node_save($node);

    drush_print(dt('changing node @nid to status @status.', array('@nid' => $nid, '@status' => $node->status)));

    usleep($sleep*1000);
  }

  //reset all nodes to the original status

  drush_print(dt('resetting all nodes to original status. please wait...'));

  foreach($touchedNids as $nid => $status) {

	$node = node_load($nid);
	if ($node->status != $status) {

		$node->status=$status;
		node_save($node);
	}
  }

  drush_print(dt('done.'));
}

function _rsttools_publish($arg) {

	_rsttools_modify_nodes_published_status($arg);
}

function _rsttools_unpublish($arg) {

        _rsttools_modify_nodes_published_status($arg, NODE_NOT_PUBLISHED);
}

/**
* Publish all nodes of a given set of types.
*
* @arg string $types
* Comma seperated list of node types.
*/
function _rsttools_modify_nodes_published_status($arg, $status = NODE_PUBLISHED) {

  $nids = _rsttools_parse_node_argument_list($arg);

  $modified = 0;
  $unchanged = 0;
  $total = sizeof($nids);

  foreach($nids as $nid) {   
   
    $node = node_load($nid);
    
    if ($node->status == $status) {
      ++$unchanged;
      continue;
    }

    $node->status = $status;
    node_save($node);
    ++$modified;
  }

  drush_print(dt('Selected @total nodes. modified @modified nodes, @unchanged nodes same status.', array(
    '@total' => $total, 
    '@modified' => $modified, 
    '@unchanged' => $unchanged,   
    'success'
 )));
}

/**
 * @return An array of valid node ids.
 */
function _rsttools_parse_node_argument_list($arg) {
  $args = explode(',', $arg);

  $nids = array();

  foreach($args as $arg) {
    
    if (array_key_exists($arg, node_type_get_names())) {
      $query = db_select('node', 'n');
      $query
        ->condition('n.type', $arg )
        ->fields('n', array('nid'));
      $result = $query->execute();
      foreach($result as $row) {
        array_push($nids,  $row->nid);
      }
    }
    else if (strpos($arg, '-') !== FALSE) { 
      $range = explode('-', $arg);
      $range[0] = intval($range[0]);
      $range[1] = intval($range[1]);

      if ($range[0] !== 0 && $range[1] !== 0) {
        $nids = array_merge($nids, range($range[0], $range[1])); 
      }
    }
    else if (intval($arg) !== 0) {
      array_push($nids, intval($arg));
    }
  }

  return $nids;
}

function _rsttools_random_nid($nids) {

	$i = mt_rand(0,sizeof($nids)-1);

	return $nids[$i];
}

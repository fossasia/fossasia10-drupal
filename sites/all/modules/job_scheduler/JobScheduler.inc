<?php
// $Id: JobScheduler.inc,v 1.6 2010/09/14 19:09:13 alexb Exp $

/**
 * @file
 * JobScheduler class.
 */

/**
 * Handle adding and removing jobs from schedule.
 */
class JobScheduler {
  /**
   * Create a single instance of JobScheduler.
   */
  public static function instance() {
    static $instance;
    if (!isset($instance)) {
      $class = variable_get('job_scheduler_class', 'JobScheduler');
      $instance = new $class();
    }
    return $instance;
  }

  /**
   * Protect constructor.
   */
  protected function __construct() {}

  /**
   * Add a job to the schedule, replace any existing job.
   *
   * A job is uniquely identified by $job = array(callbac, type, id).
   *
   * @param $job
   *   An array that must contain the following keys:
   *   'callback' - The callback to evoke.
   *   'type'     - A string identifier of the type of job.
   *   'id'       - A numeric identifier of the job.
   *   'period'   - The time when the task should be executed.
   *   'periodic' - True if the task should be repeated periodically.
   *
   * @code
   * function callback($job) {
   *   // Work off job.
   *   // Set next time to be called. If this portion of the code is not
   *   // reached for some reason, the scheduler will keep periodically invoking
   *   // the callback() with the period value initially specified.
   *   $scheduler->set($job);
   * }
   */
  public function set($job) {
    $job['last'] = JOB_SCHEDULER_REQUEST_TIME;
    $job['next'] = JOB_SCHEDULER_REQUEST_TIME + $job['period'];
    $job['scheduled'] = 0;
    $this->remove($job);
    drupal_write_record('job_schedule', $job);
  }

  /**
   * Remove a job from the schedule, replace any existing job.
   *
   * A job is uniquely identified by $job = array(callbac, type, id).
   */
  public function remove($job) {
    db_query("DELETE FROM {job_schedule} WHERE callback = '%s' AND type = '%s' AND id = %d", $job['callback'], $job['type'], isset($job['id']) ? $job['id'] : 0);
  }

  /**
   * Remove all jobs for a given callback and type.
   */
  public function removeAll($callback, $type) {
    db_query("DELETE FROM {job_schedule} WHERE callback = '%s' AND type = '%s'", $callback, $type);
    return db_affected_rows();
  }

  /**
   * Periodic cron task.
   */
  public function cron() {
    // Check and set scheduler semaphore, take time.
    if (variable_get('job_scheduler_cron', FALSE)) {
      watchdog('JobScheduler', 'Last cron process did not finish.', array(), WATCHDOG_ERROR);
    }
    variable_set('job_scheduler_cron', TRUE);
    $start = time();

    // Reschedule stuck periodic jobs after one hour.
    db_query("UPDATE {job_schedule} SET scheduled = 0 WHERE scheduled < %d AND periodic = 1", JOB_SCHEDULER_REQUEST_TIME - 3600);

    // Query and dispatch scheduled jobs.
    $num = module_exists('drupal_queue') ? variable_get('job_schedule_queue_num', 200) : variable_get('job_schedule_num', 5);
    $result = db_query_range("SELECT * FROM {job_schedule} WHERE scheduled = 0 AND next < %d ORDER BY next ASC", JOB_SCHEDULER_REQUEST_TIME, 0, $num);
    while ($job = db_fetch_array($result)) {
      // Flag periodic jobs as scheduled, remove one-off jobs.
      if ($job['periodic']) {
        $job['scheduled'] = $job['last'] = JOB_SCHEDULER_REQUEST_TIME;
        $job['next'] = $job['period'] + JOB_SCHEDULER_REQUEST_TIME;
        drupal_write_record('job_schedule', $job, array('callback', 'type', 'id'));
      }
      else {
        $this->remove($job);
      }
      // Queue job if there is a queue declared for it, otherwise execute it.
      if (function_exists($job['callback'])) {
        if (!$this->queue($job)) {
          $job['callback']($job);
        }
      }
    }

    // Unflag and post a message that we're done.
    variable_set('job_scheduler_cron', FALSE);
    watchdog('JobScheduler', 'Finished processing schedule after !time.', array('!time' => format_interval(time() - $start)));
  }

  /**
   * Add a job to a queue.
   */
  protected function queue($job) {
    if (module_exists('drupal_queue')) {
      drupal_queue_include();
      if ($queue = drupal_queue_get($job['callback'])) {
        if ($queue->createItem($job)) {
          return TRUE;
        }
      }
    }
    return FALSE;
  }
}

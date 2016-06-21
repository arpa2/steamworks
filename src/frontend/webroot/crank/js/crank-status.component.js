'use strict';

angular.
  module('crankApp').
  component('crankStatus', {
    templateUrl: 'tmpl/crank-status.template.html',
    controller: function CrankStatusController() {
      this.status = 'Not connected';
    }
  });

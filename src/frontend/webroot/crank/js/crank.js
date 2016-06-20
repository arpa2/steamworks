var crankApp = angular.module("crankApp", []);

crankApp.controller("CrankStatusController",
  function CrankStatusController($scope) {
    $scope.crank = {
      status: "Not Connected"
    }
  }
);

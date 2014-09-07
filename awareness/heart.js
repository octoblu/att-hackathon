angular.module('sqrt', ["kendo.directives"])
    .controller('HeartController', function( $scope){
        $scope.model = window.model;
        setInterval(function(){
            $scope.model.heartRate = window.model.face * 100;
            $scope.$apply()
        }, 100);
    });